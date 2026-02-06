#pragma once

#include <c10/core/DeviceType.h>
#include <pluss/SeriesBody.h>
#include <pluss/Types.h>
#include <internals/core/include/b10/deets/operations.h>
#include <pluss/Fusion.h>
#include <pluss/Series.h>
#include <pluss/rnn/rnn_cell.h>
#include <pluss/rnn/lstm_cell.h>
#include <pluss/rnn/gru_cell.h>
#include <tuple>
#include <vector>
#include <string>
#include <memory>
#include <random>

namespace pluss {
namespace rnn {

/**
 * @brief Packed sequence for handling variable-length sequences
 */
class PackedSequence {
public:
    /**
     * @brief Construct a new PackedSequence
     * 
     * @param data The packed data tensor
     * @param batch_sizes Batch sizes for each step
     * @param sorted_indices Indices to sort the sequences by length
     * @param unsorted_indices Indices to unsort the sequences
     */
    PackedSequence(const Series& data, const Series& batch_sizes,
                  const Series& sorted_indices, const Series& unsorted_indices)
        : data_(data), batch_sizes_(batch_sizes),
          sorted_indices_(sorted_indices), unsorted_indices_(unsorted_indices) {}
    
    /**
     * @brief Get the packed data tensor
     * 
     * @return const Series& Packed data tensor
     */
    const Series& data() const {
        return data_;
    }
    
    /**
     * @brief Get the batch sizes tensor
     * 
     * @return const Series& Batch sizes tensor
     */
    const Series& batch_sizes() const {
        return batch_sizes_;
    }
    
    /**
     * @brief Get the sorted indices tensor
     * 
     * @return const Series& Sorted indices tensor
     */
    const Series& sorted_indices() const {
        return sorted_indices_;
    }
    
    /**
     * @brief Get the unsorted indices tensor
     * 
     * @return const Series& Unsorted indices tensor
     */
    const Series& unsorted_indices() const {
        return unsorted_indices_;
    }
    
private:
    Series data_;
    Series batch_sizes_;
    Series sorted_indices_;
    Series unsorted_indices_;
};

/**
 * @brief Pack a padded batch of sequences
 * 
 * @param input Padded batch of sequences
 * @param lengths Sequence lengths
 * @param batch_first If true, batch dimension is first
 * @return PackedSequence Packed sequence
 */
PackedSequence pack_padded_sequence(const Series& input, const Series& lengths, bool batch_first = false) {
    // Get dimensions
    int64_t batch_size = batch_first ? input.sizes()[0] : input.sizes()[1];
    int64_t max_seq_len = batch_first ? input.sizes()[1] : input.sizes()[0];
    
    // Sort sequences by length
    Series lengths_cpu = lengths.to_cpu();
    std::vector<std::pair<int64_t, int64_t>> lengths_idx;
    for (int64_t i = 0; i < batch_size; ++i) {
        lengths_idx.push_back({lengths_cpu[i].item<int64_t>(), i});
    }
    std::sort(lengths_idx.begin(), lengths_idx.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Create sorted indices
    std::vector<int64_t> sorted_indices_vec;
    for (const auto& p : lengths_idx) {
        sorted_indices_vec.push_back(p.second);
    }
    Series sorted_indices = b10::detail::series_gen(torch::tensor(sorted_indices_vec, torch::dtype(torch::kInt64)));
    
    // Create unsorted indices
    std::vector<int64_t> unsorted_indices_vec(batch_size);
    for (int64_t i = 0; i < batch_size; ++i) {
        unsorted_indices_vec[sorted_indices_vec[i]] = i;
    }
    Series unsorted_indices = b10::detail::series_gen(torch::tensor(unsorted_indices_vec, torch::dtype(torch::kInt64)));
    
    // Sort sequences
    Series sorted_input;
    if (batch_first) {
        sorted_input = input.index_select(0, sorted_indices);
    } else {
        sorted_input = input.index_select(1, sorted_indices);
    }
    
    // Create batch sizes
    std::vector<int64_t> batch_sizes_vec;
    int64_t prev_len = lengths_idx[0].first;
    for (int64_t i = 0; i < max_seq_len; ++i) {
        int64_t current_batch_size = 0;
        for (int64_t j = 0; j < batch_size; ++j) {
            if (lengths_idx[j].first > i) {
                current_batch_size++;
            }
        }
        batch_sizes_vec.push_back(current_batch_size);
    }
    Series batch_sizes = b10::detail::series_gen(torch::tensor(batch_sizes_vec, torch::dtype(torch::kInt64)));
    
    // Pack sequences
    std::vector<Series> packed_chunks;
    int64_t offset = 0;
    for (int64_t i = 0; i < max_seq_len; ++i) {
        int64_t current_batch_size = batch_sizes_vec[i];
        if (current_batch_size == 0) {
            break;
        }
        
        Series chunk;
        if (batch_first) {
            chunk = sorted_input.narrow(0, 0, current_batch_size).narrow(1, i, 1).squeeze(1);
        } else {
            chunk = sorted_input.narrow(0, i, 1).narrow(1, 0, current_batch_size).squeeze(0);
        }
        packed_chunks.push_back(chunk);
        offset += current_batch_size;
    }
    
    // Concatenate chunks
    Series data = pluss::unite(packed_chunks, 0);
    
    return PackedSequence(data, batch_sizes, sorted_indices, unsorted_indices);
}

/**
 * @brief Pad a packed batch of sequences
 * 
 * @param sequence Packed sequence
 * @param batch_first If true, batch dimension is first
 * @return std::tuple<Series, Series> Padded batch of sequences and sequence lengths
 */
std::tuple<Series, Series> pad_packed_sequence(const PackedSequence& sequence, bool batch_first = false) {
    // Get dimensions
    const Series& data = sequence.data();
    const Series& batch_sizes = sequence.batch_sizes();
    const Series& unsorted_indices = sequence.unsorted_indices();
    
    int64_t max_seq_len = batch_sizes.sizes()[0];
    int64_t batch_size = unsorted_indices.sizes()[0];
    int64_t hidden_size = data.shape(1);
    
    // Create padded tensor
    std::vector<int64_t> padded_sizes;
    if (batch_first) {
        padded_sizes = {batch_size, max_seq_len, hidden_size};
    } else {
        padded_sizes = {max_seq_len, batch_size, hidden_size};
    }
    Series padded = pluss::zeros(padded_sizes, 
                               pluss::config({{"dtype", "int64"}, {"requires_grad", "false"}}));
    
    // Create lengths tensor
    std::vector<int64_t> lengths_vec(batch_size, 0);
    
    // Unpack sequences
    int64_t offset = 0;
    for (int64_t i = 0; i < max_seq_len; ++i) {
        int64_t current_batch_size = batch_sizes[i].item<int64_t>();
        if (current_batch_size == 0) {
            break;
        }
        
        Series chunk = data.narrow(0, offset, current_batch_size);
        
        if (batch_first) {
            padded.narrow(0, 0, current_batch_size).narrow(1, i, 1).copy_(chunk.unsqueeze(1));
        } else {
            padded.narrow(0, i, 1).narrow(1, 0, current_batch_size).copy_(chunk.unsqueeze(0));
        }
        
        for (int64_t j = 0; j < current_batch_size; ++j) {
            lengths_vec[j] = std::max(lengths_vec[j], i + 1);
        }
        
        offset += current_batch_size;
    }
    
    // Unsort sequences
    Series sorted_padded;
    if (batch_first) {
        sorted_padded = padded.index_select(0, unsorted_indices);
    } else {
        sorted_padded = padded.index_select(1, unsorted_indices);
    }
    
    // Create lengths tensor
    Series lengths = b10::detail::series_gen(torch::tensor(lengths_vec, torch::dtype(torch::kInt64)));
    
    return std::make_tuple(sorted_padded, lengths);
}

/**
 * @brief Base class for RNN modules
 */
class RNNBase {
public:
    /**
     * @brief Construct a new RNNBase
     * 
     * @param mode RNN mode ('RNN_TANH', 'RNN_RELU', 'LSTM', 'GRU')
     * @param input_size The number of expected features in the input x
     * @param hidden_size The number of features in the hidden state h
     * @param num_layers Number of recurrent layers
     * @param bias If false, then the layer does not use bias weights
     * @param batch_first If true, then the input and output tensors are provided as (batch, seq, feature)
     * @param dropout Dropout probability
     * @param bidirectional If true, becomes a bidirectional RNN
     * @param proj_size If > 0, will use LSTM with a projection layer of the given size
     */
    RNNBase(const std::string& mode, int64_t input_size, int64_t hidden_size,
           int64_t num_layers = 1, bool bias = true, bool batch_first = false,
           double dropout = 0.0, bool bidirectional = false, int64_t proj_size = 0)
        : mode_(mode), input_size_(input_size), hidden_size_(hidden_size),
          num_layers_(num_layers), bias_(bias), batch_first_(batch_first),
          dropout_(dropout), bidirectional_(bidirectional), proj_size_(proj_size) {
        
        // Validate parameters
        if (mode != "RNN_TANH" && mode != "RNN_RELU" && mode != "LSTM" && mode != "GRU") {
            throw std::invalid_argument("Invalid RNN mode: " + mode);
        }
        
        if (input_size <= 0) {
            throw std::invalid_argument("input_size must be > 0");
        }
        
        if (hidden_size <= 0) {
            throw std::invalid_argument("hidden_size must be > 0");
        }
        
        if (num_layers <= 0) {
            throw std::invalid_argument("num_layers must be > 0");
        }
        
        if (dropout < 0.0 || dropout > 1.0) {
            throw std::invalid_argument("dropout must be between 0 and 1");
        }
        
        if (proj_size < 0) {
            throw std::invalid_argument("proj_size must be >= 0");
        }
        
        if (proj_size > 0 && mode != "LSTM") {
            throw std::invalid_argument("proj_size > 0 only supported for LSTM");
        }
        
        // Initialize parameters
        initialize_parameters();
    }
    
    /**
     * @brief Forward pass
     * 
     * @param input Input tensor of shape (seq_len, batch, input_size) or (batch, seq_len, input_size)
     * @param hx Initial hidden state
     * @return std::tuple<Series, Series> Output and final hidden state
     */
    virtual std::tuple<Series, Series> forward(const Series& input, const Series& hx = Series()) const = 0;
    
    /**
     * @brief Forward pass with packed sequence
     * 
     * @param input Packed sequence
     * @param hx Initial hidden state
     * @return std::tuple<PackedSequence, Series> Output and final hidden state
     */
    virtual std::tuple<PackedSequence, Series> forward_packed(const PackedSequence& input, const Series& hx = Series()) const = 0;
    
    /**
     * @brief Get the input size
     * 
     * @return int64_t Input size
     */
    int64_t input_size() const {
        return input_size_;
    }
    
    /**
     * @brief Get the hidden size
     * 
     * @return int64_t Hidden size
     */
    int64_t hidden_size() const {
        return hidden_size_;
    }
    
    /**
     * @brief Get the number of layers
     * 
     * @return int64_t Number of layers
     */
    int64_t num_layers() const {
        return num_layers_;
    }
    
    /**
     * @brief Check if the RNN has bias
     * 
     * @return bool True if the RNN has bias, false otherwise
     */
    bool bias() const {
        return bias_;
    }
    
    /**
     * @brief Check if batch dimension is first
     * 
     * @return bool True if batch dimension is first, false otherwise
     */
    bool batch_first() const {
        return batch_first_;
    }
    
    /**
     * @brief Get the dropout probability
     * 
     * @return double Dropout probability
     */
    double dropout() const {
        return dropout_;
    }
    
    /**
     * @brief Check if the RNN is bidirectional
     * 
     * @return bool True if the RNN is bidirectional, false otherwise
     */
    bool bidirectional() const {
        return bidirectional_;
    }
    
    /**
     * @brief Get the projection size
     * 
     * @return int64_t Projection size
     */
    int64_t proj_size() const {
        return proj_size_;
    }
    
protected:
    std::string mode_;
    int64_t input_size_;
    int64_t hidden_size_;
    int64_t num_layers_;
    bool bias_;
    bool batch_first_;
    double dropout_;
    bool bidirectional_;
    int64_t proj_size_;
    
    std::vector<Series> weight_ih_l_;
    std::vector<Series> weight_hh_l_;
    std::vector<Series> bias_ih_l_;
    std::vector<Series> bias_hh_l_;
    std::vector<Series> weight_hr_l_;  // Projection weights for LSTM
    
    /**
     * @brief Initialize parameters
     */
    void initialize_parameters() {
        int64_t num_directions = bidirectional_ ? 2 : 1;
        int64_t gate_size = 0;
        
        if (mode_ == "RNN_TANH" || mode_ == "RNN_RELU") {
            gate_size = hidden_size_;
        } else if (mode_ == "LSTM") {
            gate_size = 4 * hidden_size_;
        } else if (mode_ == "GRU") {
            gate_size = 3 * hidden_size_;
        }
        
        // Initialize weights and biases
        for (int64_t layer = 0; layer < num_layers_; ++layer) {
            for (int64_t direction = 0; direction < num_directions; ++direction) {
                int64_t layer_input_size = (layer == 0) ? input_size_ : hidden_size_ * num_directions;
                
                // Input-to-hidden weights
                weight_ih_l_.push_back(pluss::randn({gate_size, layer_input_size}, 
                                                  pluss::config({{"dtype", "float"}, {"requires_grad", "false"}})));
                
                // Hidden-to-hidden weights
                int64_t hidden_size_proj = (proj_size_ > 0 && mode_ == "LSTM") ? proj_size_ : hidden_size_;
                weight_hh_l_.push_back(pluss::randn({gate_size, hidden_size_proj}, 
                                                  pluss::config({{"dtype", "float"}, {"requires_grad", "false"}})));
                
                // Biases
                if (bias_) {
                    bias_ih_l_.push_back(pluss::zeros({gate_size}, 
                                                    pluss::config({{"dtype", "float"}, {"requires_grad", "false"}})));
                    bias_hh_l_.push_back(pluss::zeros({gate_size}, 
                                                    pluss::config({{"dtype", "float"}, {"requires_grad", "false"}})));
                }
                
                // Projection weights for LSTM
                if (proj_size_ > 0 && mode_ == "LSTM") {
                    weight_hr_l_.push_back(pluss::randn({proj_size_, hidden_size_}, 
                                                      pluss::config({{"dtype", "float"}, {"requires_grad", "false"}})));
                }
            }
        }
    }
    
    /**
     * @brief Apply dropout
     * 
     * @param x Input tensor
     * @param p Dropout probability
     * @param training Whether in training mode
     * @return Series Output tensor
     */
    Series dropout_layer(const Series& x, double p, bool training = true) const {
        if (!training || p == 0.0) {
            return x;
        }
        
        Series mask = pluss::rand(x.sizes(), 
                                pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        mask = mask.gt(p);
        Series output = x.mul(mask);
        output = output.div(1.0 - p);  // Scale by 1/(1-p)
        
        return output;
    }
};

/**
 * @brief RNN module with tanh or ReLU non-linearity
 */
class RNN : public RNNBase {
public:
    /**
     * @brief Construct a new RNN
     * 
     * @param input_size The number of expected features in the input x
     * @param hidden_size The number of features in the hidden state h
     * @param num_layers Number of recurrent layers
     * @param nonlinearity The non-linearity to use. Can be 'tanh' or 'relu'. Default: 'tanh'
     * @param bias If false, then the layer does not use bias weights
     * @param batch_first If true, then the input and output tensors are provided as (batch, seq, feature)
     * @param dropout Dropout probability
     * @param bidirectional If true, becomes a bidirectional RNN
     */
    RNN(int64_t input_size, int64_t hidden_size, int64_t num_layers = 1,
       const std::string& nonlinearity = "tanh", bool bias = true, bool batch_first = false,
       double dropout = 0.0, bool bidirectional = false)
        : RNNBase(nonlinearity == "relu" ? "RNN_RELU" : "RNN_TANH", input_size, hidden_size,
                 num_layers, bias, batch_first, dropout, bidirectional, 0) {}
    
    /**
     * @brief Forward pass
     * 
     * @param input Input tensor of shape (seq_len, batch, input_size) or (batch, seq_len, input_size)
     * @param hx Initial hidden state of shape (num_layers * num_directions, batch, hidden_size)
     * @return std::tuple<Series, Series> Output and final hidden state
     */
    std::tuple<Series, Series> forward(const Series& input, const Series& hx = Series()) const override {
        // Reshape input if batch_first is true
        Series x = input;
        if (batch_first_) {
            x = x.transpose(0, 1);  // (batch, seq, input) -> (seq, batch, input)
        }
        
        // Get dimensions
        int64_t seq_len = x.sizes()[0];
        int64_t batch_size = x.sizes()[1];
        int64_t num_directions = bidirectional_ ? 2 : 1;
        
        // Initialize hidden state if not provided
        Series h = hx;
        if (!h.defined() || h.numel() == 0) {
            h = pluss::zeros({num_layers_ * num_directions, batch_size, hidden_size_}, 
                           pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        }
        
        // Initialize output
        Series output = pluss::zeros({seq_len, batch_size, hidden_size_ * num_directions}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        // Forward pass
        Series h_next = h.clone();
        
        for (int64_t layer = 0; layer < num_layers_; ++layer) {
            Series layer_output = pluss::zeros({seq_len, batch_size, hidden_size_ * num_directions}, 
                                             pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            
            for (int64_t direction = 0; direction < num_directions; ++direction) {
                int64_t idx = layer * num_directions + direction;
                Series h_prev = h.narrow(0, idx, 1).squeeze(0);
                
                // Get layer parameters
                Series weight_ih = weight_ih_l_[idx];
                Series weight_hh = weight_hh_l_[idx];
                Series bias_ih = bias_ ? bias_ih_l_[idx] : Series();
                Series bias_hh = bias_ ? bias_hh_l_[idx] : Series();
                
                // Process sequence
                for (int64_t t = 0; t < seq_len; ++t) {
                    int64_t seq_idx = direction == 0 ? t : seq_len - 1 - t;
                    Series xt = x.narrow(0, seq_idx, 1).squeeze(0);
                    
                    // Apply RNN cell
                    Series h_t = Fusion(xt, weight_ih, bias_ih).add(Fusion(h_prev, weight_hh, bias_hh));
                    
                    // Apply non-linearity
                    if (mode_ == "RNN_TANH") {
                        h_t = h_t.tanh();
                    } else if (mode_ == "RNN_RELU") {
                        h_t = h_t.relu();
                    }
                    
                    // Update hidden state
                    h_prev = h_t;
                    
                    // Update output
                    int64_t output_idx = direction == 0 ? seq_idx : seq_idx + hidden_size_;
                    layer_output.narrow(0, seq_idx, 1).narrow(2, direction * hidden_size_, hidden_size_).copy_(h_t.unsqueeze(0));
                }
                
                // Update hidden state
                h_next.narrow(0, idx, 1).copy_(h_prev.unsqueeze(0));
            }
            
            // Update input for next layer
            x = layer_output;
            
            // Apply dropout between layers
            if (layer < num_layers_ - 1 && dropout_ > 0.0) {
                x = dropout_layer(x, dropout_, true);
            }
        }
        
        // Reshape output if batch_first is true
        if (batch_first_) {
            output = output.transpose(0, 1);  // (seq, batch, hidden) -> (batch, seq, hidden)
        }
        
        return std::make_tuple(x, h_next);
    }
    
    /**
     * @brief Forward pass with packed sequence
     * 
     * @param input Packed sequence
     * @param hx Initial hidden state
     * @return std::tuple<PackedSequence, Series> Output and final hidden state
     */
    std::tuple<PackedSequence, Series> forward_packed(const PackedSequence& input, const Series& hx = Series()) const override {
        // Unpack sequence
        auto [padded, lengths] = pad_packed_sequence(input, batch_first_);
        
        // Forward pass
        auto [output, h_next] = forward(padded, hx);
        
        // Pack output
        PackedSequence output_packed = pack_padded_sequence(output, lengths, batch_first_);
        
        return std::make_tuple(output_packed, h_next);
    }
};

/**
 * @brief LSTM module
 */
class LSTM : public RNNBase {
public:
    /**
     * @brief Construct a new LSTM
     * 
     * @param input_size The number of expected features in the input x
     * @param hidden_size The number of features in the hidden state h
     * @param num_layers Number of recurrent layers
     * @param bias If false, then the layer does not use bias weights
     * @param batch_first If true, then the input and output tensors are provided as (batch, seq, feature)
     * @param dropout Dropout probability
     * @param bidirectional If true, becomes a bidirectional LSTM
     * @param proj_size If > 0, will use LSTM with a projection layer of the given size
     */
    LSTM(int64_t input_size, int64_t hidden_size, int64_t num_layers = 1,
        bool bias = true, bool batch_first = false, double dropout = 0.0,
        bool bidirectional = false, int64_t proj_size = 0)
        : RNNBase("LSTM", input_size, hidden_size, num_layers, bias, batch_first, dropout, bidirectional, proj_size) {}
    
    /**
     * @brief Forward pass
     * 
     * @param input Input tensor of shape (seq_len, batch, input_size) or (batch, seq_len, input_size)
     * @param hx Initial hidden state tuple (h_0, c_0) of shapes (num_layers * num_directions, batch, hidden_size)
     * @return std::tuple<Series, std::tuple<Series, Series>> Output and final hidden state tuple (h_n, c_n)
     */
    std::tuple<Series, std::tuple<Series, Series>> forward(const Series& input, const std::tuple<Series, Series>& hx = std::tuple<Series, Series>()) const {
        // Reshape input if batch_first is true
        Series x = input;
        if (batch_first_) {
            x = x.transpose(0, 1);  // (batch, seq, input) -> (seq, batch, input)
        }
        
        // Get dimensions
        int64_t seq_len = x.sizes()[0];
        int64_t batch_size = x.sizes()[1];
        int64_t num_directions = bidirectional_ ? 2 : 1;
        
        // Initialize hidden state if not provided
        Series h, c;
        if (std::get<0>(hx).defined() && std::get<0>(hx).numel() > 0) {
            h = std::get<0>(hx);
            c = std::get<1>(hx);
        } else {
            int64_t hidden_size_proj = (proj_size_ > 0) ? proj_size_ : hidden_size_;
            h = pluss::zeros({num_layers_ * num_directions, batch_size, hidden_size_proj}, 
                           pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            c = pluss::zeros({num_layers_ * num_directions, batch_size, hidden_size_}, 
                           pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        }
        
        // Initialize output
        int64_t hidden_size_out = (proj_size_ > 0) ? proj_size_ : hidden_size_;
        Series output = pluss::zeros({seq_len, batch_size, hidden_size_out * num_directions}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        // Forward pass
        Series h_next = h.clone();
        Series c_next = c.clone();
        
        for (int64_t layer = 0; layer < num_layers_; ++layer) {
            Series layer_output = pluss::zeros({seq_len, batch_size, hidden_size_out * num_directions}, 
                                             pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
            
            for (int64_t direction = 0; direction < num_directions; ++direction) {
                int64_t idx = layer * num_directions + direction;
                Series h_prev = h.narrow(0, idx, 1).squeeze(0);
                Series c_prev = c.narrow(0, idx, 1).squeeze(0);
                
                // Get layer parameters
                Series weight_ih = weight_ih_l_[idx];
                Series weight_hh = weight_hh_l_[idx];
                Series bias_ih = bias_ ? bias_ih_l_[idx] : Series();
                Series bias_hh = bias_ ? bias_hh_l_[idx] : Series();
                Series weight_hr = (proj_size_ > 0) ? weight_hr_l_[idx] : Series();
                
                // Process sequence
                for (int64_t t = 0; t < seq_len; ++t) {
                    int64_t seq_idx = direction == 0 ? t : seq_len - 1 - t;
                    Series xt = x.narrow(0, seq_idx, 1).squeeze(0);
                    
                    // Apply LSTM cell
                    Series gates = Fusion(xt, weight_ih, bias_ih).add(Fusion(h_prev, weight_hh, bias_hh));
                    
                    // Split gates into chunks
                    auto chunks = gates.chunk(4, 1);
                    Series in_gate = chunks[0].sigmoid();
                    Series forget_gate = chunks[1].sigmoid();
                    Series cell_gate = chunks[2].tanh();
                    Series out_gate = chunks[3].sigmoid();
                    
                    // Update cell state
                    c_prev = forget_gate.mul(c_prev).add(in_gate.mul(cell_gate));
                    
                    // Update hidden state
                    Series h_t = out_gate.mul(c_prev.tanh());
                    
                    // Apply projection if needed
                    if (proj_size_ > 0) {
                     h_t = Fusion(h_t, weight_hr, Series());
                 }
                 
                 // Update hidden state
                 h_prev = h_t;
                 
                 // Update output
                 layer_output.narrow(0, seq_idx, 1).narrow(2, direction * hidden_size_out, hidden_size_out).copy_(h_t.unsqueeze(0));
             }
             
             // Update hidden state
             h_next.narrow(0, idx, 1).copy_(h_prev.unsqueeze(0));
             c_next.narrow(0, idx, 1).copy_(c_prev.unsqueeze(0));
         }
         
         // Update input for next layer
         x = layer_output;
         
         // Apply dropout between layers
         if (layer < num_layers_ - 1 && dropout_ > 0.0) {
             x = dropout_layer(x, dropout_, true);
         }
     }
     
     // Reshape output if batch_first is true
     if (batch_first_) {
         output = output.transpose(0, 1);  // (seq, batch, hidden) -> (batch, seq, hidden)
     }
     
     return std::make_tuple(output, std::make_tuple(h_next, c_next));
 }
 
 /**
  * @brief Forward pass with packed sequence
  * 
  * @param input Packed sequence
  * @param hx Initial hidden state tuple (h_0, c_0)
  * @return std::tuple<PackedSequence, std::tuple<Series, Series>> Output and final hidden state tuple (h_n, c_n)
  */
 std::tuple<PackedSequence, std::tuple<Series, Series>> forward_packed(const PackedSequence& input, 
                                                                      const std::tuple<Series, Series>& hx = std::tuple<Series, Series>()) const {
     // Unpack sequence
     auto [padded, lengths] = pad_packed_sequence(input, batch_first_);
     
     // Forward pass
     auto [output, hidden] = forward(padded, hx);
     
     // Pack output
     PackedSequence output_packed = pack_padded_sequence(output, lengths, batch_first_);
     
     return std::make_tuple(output_packed, hidden);
 }
};

/**
* @brief GRU module
*/
class GRU : public RNNBase {
public:
 /**
  * @brief Construct a new GRU
  * 
  * @param input_size The number of expected features in the input x
  * @param hidden_size The number of features in the hidden state h
  * @param num_layers Number of recurrent layers
  * @param bias If false, then the layer does not use bias weights
  * @param batch_first If true, then the input and output tensors are provided as (batch, seq, feature)
  * @param dropout Dropout probability
  * @param bidirectional If true, becomes a bidirectional GRU
  */
 GRU(int64_t input_size, int64_t hidden_size, int64_t num_layers = 1,
    bool bias = true, bool batch_first = false, double dropout = 0.0,
    bool bidirectional = false)
     : RNNBase("GRU", input_size, hidden_size, num_layers, bias, batch_first, dropout, bidirectional, 0) {}
 
 /**
  * @brief Forward pass
  * 
  * @param input Input tensor of shape (seq_len, batch, input_size) or (batch, seq_len, input_size)
  * @param hx Initial hidden state of shape (num_layers * num_directions, batch, hidden_size)
  * @return std::tuple<Series, Series> Output and final hidden state
  */
 std::tuple<Series, Series> forward(const Series& input, const Series& hx = Series()) const override {
     // Reshape input if batch_first is true
     Series x = input;
     if (batch_first_) {
         x = x.transpose(0, 1);  // (batch, seq, input) -> (seq, batch, input)
     }
     
     // Get dimensions
     int64_t seq_len = x.sizes()[0];
     int64_t batch_size = x.sizes()[1];
     int64_t num_directions = bidirectional_ ? 2 : 1;
     
     // Initialize hidden state if not provided
     Series h = hx;
     if (!h.defined() || h.numel() == 0) {
         h = pluss::zeros({num_layers_ * num_directions, batch_size, hidden_size_}, 
                        pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
     }
     
     // Initialize output
     Series output = pluss::zeros({seq_len, batch_size, hidden_size_ * num_directions}, 
                                pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
     
     // Forward pass
     Series h_next = h.clone();
     
     for (int64_t layer = 0; layer < num_layers_; ++layer) {
         Series layer_output = pluss::zeros({seq_len, batch_size, hidden_size_ * num_directions}, 
                                          pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
         
         for (int64_t direction = 0; direction < num_directions; ++direction) {
             int64_t idx = layer * num_directions + direction;
             Series h_prev = h.narrow(0, idx, 1).squeeze(0);
             
             // Get layer parameters
             Series weight_ih = weight_ih_l_[idx];
             Series weight_hh = weight_hh_l_[idx];
             Series bias_ih = bias_ ? bias_ih_l_[idx] : Series();
             Series bias_hh = bias_ ? bias_hh_l_[idx] : Series();
             
             // Process sequence
             for (int64_t t = 0; t < seq_len; ++t) {
                 int64_t seq_idx = direction == 0 ? t : seq_len - 1 - t;
                 Series xt = x.narrow(0, seq_idx, 1).squeeze(0);
                 
                 // Apply GRU cell
                 Series gi = Fusion(xt, weight_ih, bias_ih);
                 Series gh = Fusion(h_prev, weight_hh, bias_hh);
                 
                 // Split gates into chunks
                 auto gi_chunks = gi.chunk(3, 1);
                 auto gh_chunks = gh.chunk(3, 1);
                 
                 Series reset_gate = gi_chunks[0].add(gh_chunks[0]).sigmoid();
                 Series update_gate = gi_chunks[1].add(gh_chunks[1]).sigmoid();
                 Series new_gate = gi_chunks[2].add(reset_gate.mul(gh_chunks[2])).tanh();
                 
                 // Update hidden state
                 Series h_t = update_gate.mul(h_prev).add(update_gate.neg().add(1.0).mul(new_gate));
                 
                 // Update hidden state
                 h_prev = h_t;
                 
                 // Update output
                 layer_output.narrow(0, seq_idx, 1).narrow(2, direction * hidden_size_, hidden_size_).copy_(h_t.unsqueeze(0));
             }
             
             // Update hidden state
             h_next.narrow(0, idx, 1).copy_(h_prev.unsqueeze(0));
         }
         
         // Update input for next layer
         x = layer_output;
         
         // Apply dropout between layers
         if (layer < num_layers_ - 1 && dropout_ > 0.0) {
             x = dropout_layer(x, dropout_, true);
         }
     }
     
     // Reshape output if batch_first is true
     if (batch_first_) {
         output = output.transpose(0, 1);  // (seq, batch, hidden) -> (batch, seq, hidden)
     }
     
     return std::make_tuple(x, h_next);
 }
 
 /**
  * @brief Forward pass with packed sequence
  * 
  * @param input Packed sequence
  * @param hx Initial hidden state
  * @return std::tuple<PackedSequence, Series> Output and final hidden state
  */
 std::tuple<PackedSequence, Series> forward_packed(const PackedSequence& input, const Series& hx = Series()) const override {
     // Unpack sequence
     auto [padded, lengths] = pad_packed_sequence(input, batch_first_);
     
     // Forward pass
     auto [output, h_next] = forward(padded, hx);
     
     // Pack output
     PackedSequence output_packed = pack_padded_sequence(output, lengths, batch_first_);
     
     return std::make_tuple(output_packed, h_next);
 }
};

} // namespace rnn
} // namespace pluss


#include <gtest/gtest.h>
#include <pluss/rnn.h>
#include <pluss/series_factory.h>
#include <random>
#include <iostream>

// Test fixture for RNN tests
class RNNTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set random seed for reproducibility
        std::srand(42);
        
        // Create random input data
        batch_size = 3;
        seq_len = 5;
        input_size = 10;
        hidden_size = 20;
        
        // Create input tensor
        input = create_random_series({seq_len, batch_size, input_size});
        
        // Create batch_first input tensor
        input_batch_first = input.permute({1, 0, 2});  // (batch, seq, input)
    }
    
    // Helper function to create random Series
    pluss::Series create_random_series(const std::vector<int64_t>& sizes) {
        pluss::Series series = pluss::zeros(sizes, pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
        
        // Fill with random values
        for (int64_t i = 0; i < series.numel(); ++i) {
            float random_value = static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f;  // Random between -1 and 1
            series.data_ptr<float>()[i] = random_value;
        }
        
        return series;
    }
    
    int batch_size;
    int seq_len;
    int input_size;
    int hidden_size;
    
    pluss::Series input;
    pluss::Series input_batch_first;
};

// Test RNN with tanh activation
TEST_F(RNNTest, RNNTanh) {
    // Create RNN
    pluss::rnn::RNN rnn(input_size, hidden_size, 1, "tanh", true, false, 0.0, false);
    
    // Forward pass
    auto [output, h_n] = rnn.forward(input);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test RNN with ReLU activation
TEST_F(RNNTest, RNNReLU) {
    // Create RNN
    pluss::rnn::RNN rnn(input_size, hidden_size, 1, "relu", true, false, 0.0, false);
    
    // Forward pass
    auto [output, h_n] = rnn.forward(input);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test RNN with batch_first=true
TEST_F(RNNTest, RNNBatchFirst) {
    // Create RNN
    pluss::rnn::RNN rnn(input_size, hidden_size, 1, "tanh", true, true, 0.0, false);
    
    // Forward pass
    auto [output, h_n] = rnn.forward(input_batch_first);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], batch_size);
    EXPECT_EQ(output.sizes()[1], seq_len);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test RNN with bidirectional=true
TEST_F(RNNTest, RNNBidirectional) {
    // Create RNN
    pluss::rnn::RNN rnn(input_size, hidden_size, 1, "tanh", true, false, 0.0, true);
    
    // Forward pass
    auto [output, h_n] = rnn.forward(input);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size * 2);  // bidirectional doubles the hidden size
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 2);  // num_layers * num_directions
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test RNN with multiple layers
TEST_F(RNNTest, RNNMultiLayer) {
    int num_layers = 2;
    
    // Create RNN
    pluss::rnn::RNN rnn(input_size, hidden_size, num_layers, "tanh", true, false, 0.0, false);
    
    // Forward pass
    auto [output, h_n] = rnn.forward(input);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], num_layers);
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test LSTM
TEST_F(RNNTest, LSTM) {
    // Create LSTM
    pluss::rnn::LSTM lstm(input_size, hidden_size, 1, true, false, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    pluss::Series c_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, hidden] = lstm.forward(input, std::make_tuple(h_0, c_0));
    auto [h_n, c_n] = hidden;
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Check cell state dimensions
    EXPECT_EQ(c_n.dim(), 3);
    EXPECT_EQ(c_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(c_n.sizes()[1], batch_size);
    EXPECT_EQ(c_n.sizes()[2], hidden_size);
}

// Test LSTM with batch_first=true
TEST_F(RNNTest, LSTMBatchFirst) {
    // Create LSTM
    pluss::rnn::LSTM lstm(input_size, hidden_size, 1, true, true, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    pluss::Series c_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, hidden] = lstm.forward(input_batch_first, std::make_tuple(h_0, c_0));
    auto [h_n, c_n] = hidden;
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], batch_size);
    EXPECT_EQ(output.sizes()[1], seq_len);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Check cell state dimensions
    EXPECT_EQ(c_n.dim(), 3);
    EXPECT_EQ(c_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(c_n.sizes()[1], batch_size);
    EXPECT_EQ(c_n.sizes()[2], hidden_size);
}

// Test LSTM with bidirectional=true
TEST_F(RNNTest, LSTMBidirectional) {
    // Create LSTM
    pluss::rnn::LSTM lstm(input_size, hidden_size, 1, true, false, 0.0, true);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({2, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    pluss::Series c_0 = pluss::zeros({2, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, hidden] = lstm.forward(input, std::make_tuple(h_0, c_0));
    auto [h_n, c_n] = hidden;
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size * 2);  // bidirectional doubles the hidden size
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 2);  // num_layers * num_directions
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Check cell state dimensions
    EXPECT_EQ(c_n.dim(), 3);
    EXPECT_EQ(c_n.sizes()[0], 2);  // num_layers * num_directions
    EXPECT_EQ(c_n.sizes()[1], batch_size);
    EXPECT_EQ(c_n.sizes()[2], hidden_size);
}

// Test LSTM with multiple layers
TEST_F(RNNTest, LSTMMultiLayer) {
    int num_layers = 2;
    
    // Create LSTM
    pluss::rnn::LSTM lstm(input_size, hidden_size, num_layers, true, false, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({num_layers, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    pluss::Series c_0 = pluss::zeros({num_layers, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, hidden] = lstm.forward(input, std::make_tuple(h_0, c_0));
    auto [h_n, c_n] = hidden;
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], num_layers);
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Check cell state dimensions
    EXPECT_EQ(c_n.dim(), 3);
    EXPECT_EQ(c_n.sizes()[0], num_layers);
    EXPECT_EQ(c_n.sizes()[1], batch_size);
    EXPECT_EQ(c_n.sizes()[2], hidden_size);
}

// Test GRU
TEST_F(RNNTest, GRU) {
    // Create GRU
    pluss::rnn::GRU gru(input_size, hidden_size, 1, true, false, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, h_n] = gru.forward(input, h_0);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test GRU with batch_first=true
TEST_F(RNNTest, GRUBatchFirst) {
    // Create GRU
    pluss::rnn::GRU gru(input_size, hidden_size, 1, true, true, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, h_n] = gru.forward(input_batch_first, h_0);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], batch_size);
    EXPECT_EQ(output.sizes()[1], seq_len);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test GRU with bidirectional=true
TEST_F(RNNTest, GRUBidirectional) {
    // Create GRU
    pluss::rnn::GRU gru(input_size, hidden_size, 1, true, false, 0.0, true);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({2, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, h_n] = gru.forward(input, h_0);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size * 2);  // bidirectional doubles the hidden size
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 2);  // num_layers * num_directions
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test GRU with multiple layers
TEST_F(RNNTest, GRUMultiLayer) {
    int num_layers = 2;
    
    // Create GRU
    pluss::rnn::GRU gru(input_size, hidden_size, num_layers, true, false, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({num_layers, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Forward pass
    auto [output, h_n] = gru.forward(input, h_0);
    
    // Check output dimensions
    EXPECT_EQ(output.dim(), 3);
    EXPECT_EQ(output.sizes()[0], seq_len);
    EXPECT_EQ(output.sizes()[1], batch_size);
    EXPECT_EQ(output.sizes()[2], hidden_size);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], num_layers);
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
}

// Test PackedSequence
TEST_F(RNNTest, PackedSequence) {
    // Create sequence lengths
    std::vector<int64_t> lengths_vec = {5, 3, 1};
    pluss::Series lengths = pluss::value(lengths_vec, 
                                       pluss::config({{"dtype", "int64"}, {"requires_grad", "false"}}));
    
    // Pack sequence
    pluss::rnn::PackedSequence packed = pluss::rnn::pack_padded_sequence(input, lengths, false);
    
    // Check packed sequence properties
    EXPECT_EQ(packed.data().dim(), 2);
    EXPECT_EQ(packed.batch_sizes().dim(), 1);
    EXPECT_EQ(packed.sorted_indices().dim(), 1);
    EXPECT_EQ(packed.unsorted_indices().dim(), 1);
    
    // Unpack sequence
    auto [unpacked, unpacked_lengths] = pluss::rnn::pad_packed_sequence(packed, false);
    
    // Check unpacked sequence dimensions
    EXPECT_EQ(unpacked.dim(), 3);
    EXPECT_EQ(unpacked.sizes()[0], seq_len);
    EXPECT_EQ(unpacked.sizes()[1], batch_size);
    EXPECT_EQ(unpacked.sizes()[2], input_size);
    
    // Check unpacked lengths
    EXPECT_EQ(unpacked_lengths.dim(), 1);
    EXPECT_EQ(unpacked_lengths.sizes()[0], batch_size);
}

// Test RNN with PackedSequence
TEST_F(RNNTest, RNNWithPackedSequence) {
    // Create RNN
    pluss::rnn::RNN rnn(input_size, hidden_size, 1, "tanh", true, false, 0.0, false);
    
    // Create sequence lengths
    std::vector<int64_t> lengths_vec = {5, 3, 1};
    pluss::Series lengths = pluss::value(lengths_vec, 
                                       pluss::config({{"dtype", "int64"}, {"requires_grad", "false"}}));
    
    // Pack sequence
    pluss::rnn::PackedSequence packed = pluss::rnn::pack_padded_sequence(input, lengths, false);
    
    // Forward pass
    auto [output_packed, h_n] = rnn.forward_packed(packed);
    
    // Check output dimensions
    EXPECT_EQ(output_packed.data().dim(), 2);
    EXPECT_EQ(output_packed.batch_sizes().dim(), 1);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Unpack output
    auto [unpacked, unpacked_lengths] = pluss::rnn::pad_packed_sequence(output_packed, false);
    
    // Check unpacked output dimensions
    EXPECT_EQ(unpacked.dim(), 3);
    EXPECT_EQ(unpacked.sizes()[0], seq_len);
    EXPECT_EQ(unpacked.sizes()[1], batch_size);
    EXPECT_EQ(unpacked.sizes()[2], hidden_size);
}

// Test LSTM with PackedSequence
TEST_F(RNNTest, LSTMWithPackedSequence) {
    // Create LSTM
    pluss::rnn::LSTM lstm(input_size, hidden_size, 1, true, false, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    pluss::Series c_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Create sequence lengths
    std::vector<int64_t> lengths_vec = {5, 3, 1};
    pluss::Series lengths = pluss::value(lengths_vec, 
                                       pluss::config({{"dtype", "int64"}, {"requires_grad", "false"}}));
    
    // Pack sequence
    pluss::rnn::PackedSequence packed = pluss::rnn::pack_padded_sequence(input, lengths, false);
    
    // Forward pass
    auto [output_packed, hidden] = lstm.forward_packed(packed, std::make_tuple(h_0, c_0));
    auto [h_n, c_n] = hidden;
    
    // Check output dimensions
    EXPECT_EQ(output_packed.data().dim(), 2);
    EXPECT_EQ(output_packed.batch_sizes().dim(), 1);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Check cell state dimensions
    EXPECT_EQ(c_n.dim(), 3);
    EXPECT_EQ(c_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(c_n.sizes()[1], batch_size);
    EXPECT_EQ(c_n.sizes()[2], hidden_size);
    
    // Unpack output
    auto [unpacked, unpacked_lengths] = pluss::rnn::pad_packed_sequence(output_packed, false);
    
    // Check unpacked output dimensions
    EXPECT_EQ(unpacked.dim(), 3);
    EXPECT_EQ(unpacked.sizes()[0], seq_len);
    EXPECT_EQ(unpacked.sizes()[1], batch_size);
    EXPECT_EQ(unpacked.sizes()[2], hidden_size);
}

// Test GRU with PackedSequence
TEST_F(RNNTest, GRUWithPackedSequence) {
    // Create GRU
    pluss::rnn::GRU gru(input_size, hidden_size, 1, true, false, 0.0, false);
    
    // Create initial hidden state
    pluss::Series h_0 = pluss::zeros({1, batch_size, hidden_size}, 
                                   pluss::config({{"dtype", "float"}, {"requires_grad", "false"}}));
    
    // Create sequence lengths
    std::vector<int64_t> lengths_vec = {5, 3, 1};
    pluss::Series lengths = pluss::value(lengths_vec, 
                                       pluss::config({{"dtype", "int64"}, {"requires_grad", "false"}}));
    
    // Pack sequence
    pluss::rnn::PackedSequence packed = pluss::rnn::pack_padded_sequence(input, lengths, false);
    
    // Forward pass
    auto [output_packed, h_n] = gru.forward_packed(packed, h_0);
    
    // Check output dimensions
    EXPECT_EQ(output_packed.data().dim(), 2);
    EXPECT_EQ(output_packed.batch_sizes().dim(), 1);
    
    // Check hidden state dimensions
    EXPECT_EQ(h_n.dim(), 3);
    EXPECT_EQ(h_n.sizes()[0], 1);  // num_layers
    EXPECT_EQ(h_n.sizes()[1], batch_size);
    EXPECT_EQ(h_n.sizes()[2], hidden_size);
    
    // Unpack output
    auto [unpacked, unpacked_lengths] = pluss::rnn::pad_packed_sequence(output_packed, false);
    
    // Check unpacked output dimensions
    EXPECT_EQ(unpacked.dim(), 3);
    EXPECT_EQ(unpacked.sizes()[0], seq_len);
    EXPECT_EQ(unpacked.sizes()[1], batch_size);
    EXPECT_EQ(unpacked.sizes()[2], hidden_size);
}

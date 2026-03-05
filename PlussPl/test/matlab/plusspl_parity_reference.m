function plusspl_parity_reference(out_path)
% Build strict MATLAB references for PlussPl parity tests.
%
% Output format (line oriented):
% key=value1,value2,...

if nargin ~= 1
    error('plusspl_parity_reference requires exactly one output path.');
end

fig = figure('Visible', 'off');
cleanup = onCleanup(@() close(fig)); %#ok<NASGU>
ax = axes(fig);

plot(ax, [1 2 3], [1 4 9], 'LineWidth', 1.2);
xlim(ax, [0.8 3.2]);
ylim(ax, [0.5 9.5]);
xticks(ax, 'auto');
yticks(ax, 'auto');
linear_xticks = xticks(ax);
linear_yticks = yticks(ax);

set(ax, 'XScale', 'log');
xlim(ax, [1e-1 1e3]);
xticks(ax, 'auto');
log_xticks = xticks(ax);

view(ax, 3);
[az3, el3] = view(ax);
view3 = [az3, el3];
view(ax, 2);
[az2, el2] = view(ax);
view2 = [az2, el2];

hist_data = [-1.8 -1.2 -0.8 -0.25 0.0 0.15 0.35 0.9 1.1 1.8];
[hist_counts, hist_edges] = histcounts(hist_data, 5, 'BinLimits', [-2, 2]);
quartiles = quantile(hist_data, [0.25 0.50 0.75]);

fid = fopen(out_path, 'w');
if fid < 0
    error('Unable to open output file: %s', out_path);
end

write_vector(fid, 'linear_xticks', linear_xticks);
write_vector(fid, 'linear_yticks', linear_yticks);
write_vector(fid, 'log_xticks', log_xticks);
write_vector(fid, 'view3', view3);
write_vector(fid, 'view2', view2);
write_vector(fid, 'hist_counts', hist_counts);
write_vector(fid, 'hist_edges', hist_edges);
write_vector(fid, 'quartiles', quartiles);
fclose(fid);
end

function write_vector(fid, key, values)
fprintf(fid, '%s=', key);
for i = 1:numel(values)
    if i > 1
        fprintf(fid, ',');
    end
    fprintf(fid, '%.16g', values(i));
end
fprintf(fid, '\n');
end

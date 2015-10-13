function [degreeDist, eff, meanFields] = singleRateUniversalOpt(M, q, rank, n, eta, epsilon, it)
% SINGLERATEUNIVERSALOPT Find universal degree distribution for a single
% channel capacity.
% 
% Example:
%  [dist, eff, meanF] = singleRateUniversalOpt(16, 256, 10, 200, 0.99, 0.002, 3500)
% 
% Input:
%  M - batch size
%  q - number of finite field elements
%  rank - expected rank of the channel
%  n - number of samples for the mean field function
%  eta - precode triggering threshold (BP will decode up to eta portion of all)
%  epsilon - uniform lower bound for the mean field function
%  it - maximum number of iterations
% Output:
%  degreeDist - Optimized degree distribution
%  eff - efficiency, i.e. percentage of the theoretical capacity achieved
%  meanFields - value of the mean field functions at all sample points, a
%  l-by-n matrix whose i-th row are the samples for the i-th vertex
% 
% See also:
%  RAWDEGREEDISTOPTIMIZATION

D = ceil(M/(1-eta));

V = vertexConstRank(M, rank);
tic
disp('Constructing mean field samples...');
B = simultaneousMeanFieldSample(M, q, D, n, eta/(n-1), V');
toc

tic
disp('Optimizing degree distribution...');
[degreeDist, eff, t] = rawDegreeDistOptimization(D, epsilon, B, it);
toc

meanFields = reshape(t, n, [])';

end

function hbar = averageRank(M, rankDists)
% AVERAGERANK Compute the average rank of multiple rank distributions at
% the same time.
% 
% Input:
%  M - batch size
%  rankDists - A m-by-(M+1) matrix whose i-th row is the i-th rank
%  distribution.
% Output:
%  hbar - A column vector of length m; the i-th entry is the average rank
%  of the i-th distribution.
m = size(rankDists, 1);
tmp = kron(ones(m, 1), 0:M).*rankDists;
hbar = sum(tmp, 2);
end

function cap = eraChainCap(M, e, l)
% ERACHAINCAP Compute network capacity of a chain of erasure channel.
% 
% Let X_1, ..., X_l be iid Binomial(M, 1-e) random variables, representing
% realized capacity of each link. Then the realized capacity of the whole
% chain is min(X_1, ..., X_l) and hence the capacity is E[min(X_1, ...,
% X_l)].
% 
% Input:
%  M - Number of packets per batch
%  e - Error rate of each channel
%  l - Number of links
% Output:
%  cap - Unnormalized network capacity

df = cumsum(mybinom(M, 1-e));
tmp = 1 - (1-df).^l;
newpmf = tmp - [0 tmp(1:end-1)];
cap = [0:M]*newpmf';
end

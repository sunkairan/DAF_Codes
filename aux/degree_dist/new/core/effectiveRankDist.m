function z = effectiveRankDist(M, q, rankDist)
% EFFECTIVERANKDIST Transform the raw rank distribution into one that takes
% into account the effect of generator matrix.
% 
% Input:
%  M - batch size
%  q - number of finite field element
%  rankDist - The rank distribution of the transfer matrix H. A vector of
%  length M+1.
% Output:
%  z - A vector of length M such that the r-th entry is sum_{i=r}^M
%  zeta_r^i  q^{r-i} h_i, where zeta_r^i := (1-q^{-i})(1-q^{-i+1})...
%  (1-q^{-i+r-1}).
z = zeros(1, M);

% TODO: Use brute force here
for r=1:M,
    for i=r:M,
        z(r) = z(r) + ufallingQFactorial(i, r, q) * q^(r-i) * rankDist(i+1);
    end
end
end

% Unnormalized falling Q factorial
function zeta = ufallingQFactorial(n, k, q)
zeta = prod(1 - (1/q^n)*geomSeq(1/q, k));
end

% [1, a, a^2, ..., a^(n-1)], length n
function v = geomSeq(a, n)
v = [1, cumprod(a * ones(1, n-1))];
end

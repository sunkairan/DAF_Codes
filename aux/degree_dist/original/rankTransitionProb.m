function zeta = rankTransitionProb(M, q, dmax)
% RANKTRANSITIONPROB Calculate rank distribution of the final matrix given
% the rank of the transfer matrix.
% 
% Let G and H be d-by-M and M-by-M F_q matrices, where G is iid uniformly
% random and H is arbitary. Then define zeta^{d,k}_r := Pr( rk(GH) = r |
% rk(H) = k, G has d rows).
% 
% Input:
%  M - Number of packets per batch / dimension of H
%  q - Number of element of finite field
%  dmax - maximum value of d
% Output:
%  zeta - Three dimensional array, such that zeta(k+1, r+1, d) is the computed
%  value. Note that 1 <= d <= dmax, 0 <= r <= k <= M.

zeta = zeros(M+1, M+1, dmax);
% aux var
f2 = tril(q.^toeplitz(0:-1:-M, zeros(1, M+1)), 0);
f1 = tril(1 - f2/q, 0);

% init cond
zeta(1:(M+1), 1, 1) = geomSeq(1/q, M+1);
zeta(1:(M+1), 2, 1) = 1 - zeta(1:(M+1), 1, 1);

% use recurrence relation
for d = 2:dmax,
    tmp = circshift(zeta(:,:,d-1), [0, 1]);
    tmp(M+1, 1) = 0;
    zeta(:,:,d) = f1.*tmp + f2.*zeta(:,:,d-1);
end
end

% [1, a, a^2, ..., a^(n-1)], length n
function v = geomSeq(a, n)
v = [1, cumprod(a * ones(1, n-1))];
end

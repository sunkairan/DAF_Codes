function A = meanFieldSample(M, q, D, n, s, rankDist)
% MEANFIELDSAMPLE Discretize the mean field trajectory.
% 
% Input:
%  D - maximum degree in the setup
%  n - number of sample
%  s - step size of sampling
% Output:
%  A - An array with dimension n-by-(D+1) such that for x = [phi_1; phi_2; 
%  ...; phi_D; theta], A*x is a column vector (of length n) of the sampled 
%  mean field function, namely [pho(0); pho(s); pho(2s); ...; pho((n-1)*s)]
% 
% See also:
%  RECVRANKPROB

h = recvRankProb(M, q, M+1, rankDist);

constHpart = zeros(1, D);
constHpart(1:M) = diag(h, -1)'.*(1:M);

alpha = (1 - 1/q)./(1 - (1/q^2)*geomSeq(1/q, M));

t = diag(h, 0);
hStar = t(2:end)'.*alpha;

% has to use loop because betainc() is not fully parallelizable :(
myBeta = zeros(M, D, n);
for i=1:M,
    for j=1:D,
        if j > i
            myBeta(i,j,:) = j*betainc(s*(0:(n-1)), j-i, i);
        end
    end
end

tt = repmat(hStar', [1, D, n]);
B = permute(sum(myBeta.*tt, 1), [3, 2, 1]) + kron(constHpart, ones(n, 1));

%post process: add parameter theta, and rescale
A = [B log(1 - s*(0:(n-1)))'];
A = A.*kron((1 - s*(0:(n-1)))', ones(1, D+1));

end

% [1, a, a^2, ..., a^(n-1)], length n
function v = geomSeq(a, n)
v = [1, cumprod(a * ones(1, n-1))];
end

function A = simultaneousMeanFieldSample(M, q, D, n, s, rankDists)
% SIMULTANEOUSMEANFIELDSAMPLE Discretize the mean field trajectory for
% multiple rank distributions at the same time.
% 
% Input:
%  D - maximum degree in the setup
%  n - number of sample
%  s - step size of sampling
% Output:
%  A - An array with dimension nm-by-(D+1) such that for x = [phi_1; phi_2;
%  ...; phi_D; r], A*x is a column vector (of length nm) of the sampled
%  mean field functions in sequential format, namely [pho_1(0); pho_1(s);
%  ...; pho_1((n-1)*s); pho_2(0); pho_2(s); ...; pho_2((n-1)*s); pho_3(0);
%  ......; pho_m((n-1)*s)]. m is the number of rank distributions present 
%  at the same time.
% 
% See also:
%  EFFECTIVERANKDIST
%  AVERAGERANK

m = size(rankDists, 1);

% has to use loop because betainc() is not fully parallelizable :(
myBeta = zeros(M, D, n);
for i=1:M,
    for j=1:D,
        if j > i
            myBeta(i,j,:) = j*betainc(s*(0:(n-1)), j-i, i);
        else
            myBeta(i,j,:) = j;
        end
    end
end

% compute the growth part
B = [];
for k=1:m,
    z = effectiveRankDist(M, q, rankDists(k,:));
    t = repmat(z', [1, D, n]);
    B = [B; permute(sum(myBeta.*t, 1), [3, 2, 1])];
end

% compute the elimination part
tt = kron(averageRank(M, rankDists), ones(n, 1)).*repmat(log(1 - s*(0:(n-1)))', [m 1]);
A = [B tt];

% post process: rescale
A = A.*repmat(kron((1 - s*(0:(n-1)))', ones(1, D+1)), [m, 1]);

end

function h = recvRankProb(M, q, dmax, rankDist)
% RECVRANKPROB Calculate unconditional rank distribution of the final
% matrix.
% 
% Input:
%  rankDist - The rank distribution of the transfer matrix H. A vector of
%  length M+1.
% Output:
%  h - An array such that h(r+1, d) = Pr(final rank = r | degree = d). The
%  range is 0 <= r <= M, 1 <= d <= dmax.
% 
% See also:
%  RANKTRANSITIONPROB

%h = zeros(M+1, dmax);

t = repmat(rankDist', [1, M+1, dmax]);
zeta = rankTransitionProb(M, q, dmax);
h = permute(sum(zeta.*t, 1), [2, 3, 1]);
end

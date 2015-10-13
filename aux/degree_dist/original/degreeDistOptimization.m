function [degreeDist, theta, meanField] = degreeDistOptimization(M, q, D, eta, n, epsilon, rankDist)
% DEGREEDISTOPTIMIZATION
%
% Input:
%  M - batch size
%  q - number of finite field element
%  D - maximum degree allowed
%  eta - precode triggering threshold (BP will decode up to eta portion of all)
%  n - number of samples for the mean field function
%  epsilon - uniform lower bound for the mean field function
%  rankDist - Empirical rank distribution
% Output:
%  degreeDist - Optimized degree distribution
%  theta - achived coding rate (unnormalized by M)
%  meanField - value of the mean field function at all sample points

B = meanFieldSample(M, q, D, n, eta/(n-1), rankDist);

%various parameters to be feed to linear programming solver
f = [zeros(D, 1); -1];
A = -1*B;
b = -epsilon*ones(n, 1);
lb = zeros(D+1, 1);

Aeq = [ones(1, D), 0];
beq = [1];

options = optimset('MaxIter', 170);

[x,~,exitflag] = linprog(f, A, b, Aeq, beq, lb, [], 0, options);

if exitflag ~= 1
    error('Linear programming solver failed to find a solution!')
end

degreeDist = x(1:D)';
theta = x(D+1);
meanField = (B*x)';

end

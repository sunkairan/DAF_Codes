function [degreeDist, r, meanFields] = rawDegreeDistOptimization(D, epsilon, coefs, it)
% RAWDEGREEDISTOPTIMIZATION The raw optimization routine when given the
% coefficients matrix.
% 
% Input:
%  D - maximum degree allowed
%  epsilon - uniform lower bound for the mean field function
%  coefs - externally supplied coefficients matrix
%  it - maximum number of iterations
% Output:
%  degreeDist - Optimized degree distribution
%  r - achived rate / optimal value
%  meanField - value of the mean field functions at all sample points

%various parameters to be feed to linear programming solver
f = [zeros(D, 1); -1];
A = -1*coefs;
b = -epsilon*ones(size(coefs, 1), 1);
lb = zeros(D+1, 1);

Aeq = [ones(1, D), 0];
beq = [1];

%options = optimset('MaxIter', it);
options = optimset('MaxIter', it, 'LargeScale', 'off', 'Simplex', 'on');

[x, ~, exitflag] = linprog(f, A, b, Aeq, beq, lb, [], 0, options);

if exitflag ~= 1
    error('Linear programming solver failed to find a solution!')
end

% set outputs
degreeDist = x(1:D)';
r = x(D+1);
meanFields = (coefs*x)';

end

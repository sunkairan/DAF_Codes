function dist = mybinom(n, p)
% MYBINOM Generate binomial distribution pmf
% 
% Input:
%  n - Number of trials
%  p - Probability of success
% Output:
%  dist - A row vector of length n+1, whose i-th entry is the probability
%  of having exactly i-1 successes

tmp = [1 cumprod((n - [0:(n-1)])./[1:n])];
dist = tmp.*(p.^[0:n]).*((1-p).^[n:-1:0]);
end

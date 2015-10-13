function V = vertexConstRank(M, r)
% VERTEXCONSTRANK List all vertices of the polytope of rank distributions
% with constant average rank
% 
% Let H := {(h_1, ..., h_M) in R^{M+1} | h_i >= 0 for all i, sum_{i=0}^M
% h_i = 1} be the space of valid rank distribution, which is a convex set
% in an affine space. Define H_r to be H restricted to those distributions
% that has average rank equals to r, i.e. sum_{i=0}^M i*h_i = r. This
% function enumerates all vertices of H_r.
% 
% To do so, we searches through the 2-boundaries and 1-boundaries so that
% the equality constraints give unique solution, a necessary condition for
% the point to be a vertex. (as there are only 2 constraints, higher
% dimensional boundaries will never have unique solutions)
% 
% Input:
%  M - batch size
%  r - the fixed average rank
% Output:
%  V - A (M+1)-by-l matrix, where each column gives a vertex.

L = floor(r);
U = ceil(r);

if (L == U)
    %r is integer, has an extra vertex on the 1-boundary
    %Change the range and preallocate
    L = floor(r) - 1;
    U = floor(r) + 1;
    V = zeros(M+1, (L+1)*(M-U+1) + 1);
    %the extra vertex at the end
    V(floor(r) + 1, (L+1)*(M-U+1) + 1) = 1;
else
    V = zeros(M+1, (L+1)*(M-U+1));
end

%Enumerate all vertices on 2-boundary; must have i < r < j for positive h_k
c = 1;
for i=0:L,
    for j=U:M,
        V([i+1 j+1], c) = [j-r r-i]/(j-i);
        c = c+1;
    end
end
end

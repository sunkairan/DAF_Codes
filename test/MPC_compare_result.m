% Output the variance comparisons and schedulers for combinations of predictive optimization.
%   The txt file is designed for easy excel import.
%   The input matches test_all_predictive_len
% Input:
%   seqName: name of the sequence
%   factors: all the factors of prediction want to loop;
%       factor: 0 is cheating, non-opt or full-opt (means N/A);
%   optLen: 0 is non-opt (original); T is full-opt;
%   full opt: optLen = T & cheating = 2 & factor = 0;
%   non-opt: optLen = 0 & cheating = 0 & factor = 0;
%   outputSchedulers: switch for wheather to output scheduler.
% see also: test_all_predictive_len


seqName= 'akiyo';
dt = 1;
cheat1 = 0; 
factors = [0.5 0.75 1]; % prediction factor
allW = [20:5:45];
maxOptLen = 20;

outputSchedulers = 1;

load(strcat(seqName, '_seq_br.mat'));
norm_s = s./(sum(s)/size(s,2));
T = size(norm_s,2);
fileName = [seqName '_NMPC_results.txt'];

fileID = fopen (fileName ,'w');
fprintf(fileID, 'seqName\tdt\tcheat?\tfactor\tW\toptLen\tvariance\tuniform\toptimal\n');

% print non-opt 
for W = allW
    optLen = 0;
    cheating = 0;
    factor = 0;
    load(['FO_OptResult_T' num2str(T) '_W' num2str(W) '_dt' num2str(dt) '_' seqName '.mat']);
    [X , Xnoint] = getXFromx( x, T, W, dt, norm_s );
    [X0 , Xnoint] = getXFromx( x0, T, W, dt, norm_s );
    fprintf(fileID, '%s\t%d\t%d\t%f\t%d\t%d\t%f\t%f\t%f\n',seqName,dt,cheating,factor,W,optLen,var(X0(W:T-W+1)),var(X0(W:T-W+1)),var(X(W:T-W+1)));
end

% print predict opt with cheating
cheating = 2;
factor = 0;
for W = allW
    for optLen = [1 [5:5:min(W,maxOptLen)]]
        outputPrefix = ['NMPC_Cheat_L' num2str(optLen) '_OptResult_F' num2str(factor) '_'];
        load([outputPrefix 'T' num2str(T) '_W' num2str(W) '_dt' num2str(dt) '_' seqName '.mat']);
        [X , Xnoint] = getXFromx( x, T, W, dt, norm_s );
        fprintf(fileID, '%s\t%d\t%d\t%f\t%d\t%d\t%f\t',seqName,dt,cheating,factor,W,optLen,var(X(W:T-W+1)));
        if(outputSchedulers)
            postfixName = ['_NMPC_L',int2str(optLen) , '_F',num2str(factor) ];
            genPacketScheduler_predict;
        end
        load(['FO_OptResult_T' num2str(T) '_W' num2str(W) '_dt' num2str(dt) '_' seqName '.mat']);
        [X , Xnoint] = getXFromx( x, T, W, dt, norm_s );
        [X0 , Xnoint] = getXFromx( x0, T, W, dt, norm_s );
        fprintf(fileID, '%f\t%f\n',var(X0(W:T-W+1)),var(X(W:T-W+1)));
    end
end

% print predict opt 
cheating = 0;
for factor = factors
    for W = allW
        for optLen = [1 [5:5:min(W,maxOptLen)]]
            outputPrefix = ['NMPC_L' num2str(optLen) '_OptResult_F' num2str(factor) '_'];
            load([outputPrefix 'T' num2str(T) '_W' num2str(W) '_dt' num2str(dt) '_' seqName '.mat']);
            [X , Xnoint] = getXFromx( x, T, W, dt, norm_s );
            fprintf(fileID, '%s\t%d\t%d\t%f\t%d\t%d\t%f\t',seqName,dt,cheating,factor,W,optLen,var(X(W:T-W+1)));
            if (outputSchedulers)
                postfixName = ['_NMPC_L',int2str(optLen) , '_F',num2str(factor) ];
                genPacketScheduler_predict;
            end
            load(['FO_OptResult_T' num2str(T) '_W' num2str(W) '_dt' num2str(dt) '_' seqName '.mat']);
            [X , Xnoint] = getXFromx( x, T, W, dt, norm_s );
            [X0 , Xnoint] = getXFromx( x0, T, W, dt, norm_s );
            fprintf(fileID, '%f\t%f\n',var(X0(W:T-W+1)),var(X(W:T-W+1)));
        end
    end
end

% print full-opt 
for W = allW
    optLen = T;
    cheating = 2;
    factor=0;
    load(['FO_OptResult_T' num2str(T) '_W' num2str(W) '_dt' num2str(dt) '_' seqName '.mat']);
    [X , Xnoint] = getXFromx( x, T, W, dt, norm_s );
    [X0 , Xnoint] = getXFromx( x0, T, W, dt, norm_s );
    fprintf(fileID, '%s\t%d\t%d\t%f\t%d\t%d\t%f\t%f\t%f\n',seqName,dt,cheating,factor,W,optLen,var(X(W:T-W+1)),var(X0(W:T-W+1)),var(X(W:T-W+1)));
end

fclose(fileID);

fprintf('All results written to %s.\n',fileName);
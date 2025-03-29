-module(line).

-export([start/1, send/2, stop/1]).

-export([init/2]).


%%% API %%%


% Creates a sequence of N linked processes and returns the PID of the first process in the sequence.
start(N) when N > 0 ->
    spawn(?MODULE, init, [0, N-1]).
    
% Sends a message to the sequence of processes and returns 'ok'.
% Pid: the PID of the first process in the sequence.
% Msg: the message to be sent (Erlang Term).
send(Pid, Msg) ->
    Pid ! {send, Msg},
    ok.  

% Sends a stop signal to all processes in the sequence.
% Pid: the PID of the first process in the sequence.
stop(Pid) ->
    Pid ! stop,
    ok.

%%% INTERNAL FUNCTIONS %%%

% Initializes a process in the sequence.
% Id: the process identifier (starting from 0).
% RemainingProcs: number of remaining processes to be created.
init(Id, 0) ->
    loop(Id, none);
init (Id, RemainingProcs) ->
    NextPid = spawn (?MODULE, init, [Id + 1, RemainingProcs - 1]),
    loop (Id, NextPid).

% The main loop for each process.
loop(Id, NextPid) ->
    receive
        stop ->
            case NextPid of
                none -> ok;
                _ -> stop(NextPid)
            end;
        {send, Msg} ->
            io:format("~w received message ~p~n", [Id, Msg]),  
            case NextPid of
                none -> ok;
                _ -> send (NextPid, Msg)
            end,
            loop (Id, NextPid)
    end.

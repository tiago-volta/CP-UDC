-module(ring).

-export([start/1, send/3, stop/1]).

-export([init/2]).

% Creates a ring of N linked processes and returns the PID of the first process in the ring.
start(N) when N > 0 ->
    FirstPid = spawn(?MODULE, init, [0, N-1]),
    FirstPid ! {set_last, FirstPid},        % The first process needs to know the PID of the last process to close the ring.
    FirstPid.

% Sends a message through the process ring and returns ok after sending the message.
% Each process prints its ID, the remaining count, and the received message.
% Pid: the PID of the first process in the ring.
% N: the number of times the message should circulate.
% Msg: the message to be sent (erlang term).
send(Pid, N, Msg) ->
    Pid ! {send, N, Msg},
    ok.
    
% Stops all processes in the ring.
% Pid: the PID of the first process in the ring.
stop(Pid) ->
    Pid ! stop,
    ok.

%%% INTERNAL FUNCTIONS %%%

% Initializes a process in the ring.
% Id: the process identifier (starting from 0).
% RemainingProcs: the number of processes still to be created.
init(Id, 0) ->
    receive                                % The last process in the ring waits to receive the first process PID to complete the ring.
        {set_last, FirstPid} ->
            loop(Id, FirstPid, FirstPid)
    end;
init(Id, RemainingProcs) ->
   NextPid = spawn(?MODULE, init, [Id+1, RemainingProcs-1]),
   receive
       {set_last, FirstPid} ->
           NextPid ! {set_last, FirstPid},    % Forward the last PID to the next process in the ring.
           loop(Id, NextPid, FirstPid)
    end.

% Main loop of each process in the ring.
loop(Id, NextPid, FirstPid) ->
    receive
        stop ->
            case self() =/= FirstPid of
                true ->                % Continue the chain
                    stop(NextPid);
                false ->               % If this is the first process, the stop signal has completed the ring.
                    ok
            end;
        {send, 0, _} ->
            loop(Id, NextPid, FirstPid);
        {send, N, Msg} ->
            io:format("~w receiving message ~p with ~w left~n", [Id, Msg, N-1]),
            send (NextPid, N-1, Msg),
            loop(Id, NextPid, FirstPid)
    end.            

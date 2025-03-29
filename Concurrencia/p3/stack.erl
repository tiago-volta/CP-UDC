-module(stack).

-export([empty/0, push/2, pop/1, peek/1]).

% Creates an empty stack
empty() -> [].

% Adds one element to the stack
push(Stack, Elem) -> [Elem | Stack].

% Deletes the peek element
pop([]) ->
    [];
pop([_ | Rest]) ->
    Rest.

% Returns the tuple {ok, Elem} where Elem is the peek element
peek([]) ->
    empty;
peek([Top | _]) ->
     {ok, Top}.

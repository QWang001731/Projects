This project implemented a twitter server-client system.
The implementation uses linked list as the concurrent data structure that alows different operation on the data structure to be executed
in parallel. Two versions of linked list are implemented.One is coarse grained linked list , and the other is lock free linked list.
The implementatin uses a task queue technique to distribute tasks among worker threads.



# queue-rate
Queue rate testing


Change the number of producers/consumers by defining N_CONSUMERS and
N_PRODUCERS. This can be done at configure or on each make invocation:

	CPPFLAGS="-DN_CONSUMERS=1 -DN_PRODUCERS=4" configure

		-or-

	CPPFLAGS="-DN_CONSUMERS=1 -DN_PRODUCERS=4" make

The default optimization (CFLAGS) is set to "-O3". Alternative CFLAGS can be
specified at configure or on each make invokation:

	CFLAGS="-O0 -g" configure

		-or-

	CFLAGS="-O0 -g" make




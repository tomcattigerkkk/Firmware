px4_add_module(
	MODULE drivers__zigbee
	MAIN zigbee
	STACK_MAIN 2000
	SRCS
		zigbee_uart.c
	DEPENDS
		platforms__common
	)
# vim: set noet ft=cmake fenc=utf-8 ff=unix : 

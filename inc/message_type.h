#pragma once

enum MessageType : char
{
	CONNECT, DISCONNECT, CONNECT_ACK, DATA, PING, PONG
};
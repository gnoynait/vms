#include<stdio.h>
#include"vmm.h"
#include"communicate.h"
char Buffer[BUFF_SIZE];
request_t Request;
bool Continue = TRUE;
char *HELP_INFO = "WELCOME TO VMACHINE!\n"
	"Commands:\n"
	"    Q:         quit this program.\n"
	"    R xxxx:    read a byte from address xxxx.\n"
	"    W xxxx xx: write a value xx to address xxxx.\n"
	"    X xxxx:    execute a instruction at xxxx.\n"
	"    E n:       execute program n.\n"
	"    D:         display page table info.\n"
	"    S:         shut down the virtual machine.\n";


/**
 * parse a command line
 * return TRUE if everty thing is OK, FALSE if wrong.
 */
static bool parse_command()
{
	unsigned value;
	unsigned address;
	printf("$ ");
	fgets(Buffer, BUFF_SIZE - 1, stdin);
	switch (Buffer[0]) {
	case 'Q':
	case 'q':
		Request.command = QUIT;
		Continue = FALSE;
		break;
	case 'S':
	case 's':
		Request.command = SHUT_DOWN;
		Continue = FALSE;
		break;
	case 'R':
	case 'r':
		Request.command = READ;
		if ((sscanf (&Buffer[1], "%x", &address)) != 1)
			goto wrong;
		Request.address = address;
		break;
	case 'X':
	case 'x':
		Request.command = EXEC;
		if ((sscanf (&Buffer[1], "%x", &address)) != 1)
			goto wrong;
		Request.address = address;
		break;
	case 'W':
	case 'w':
		Request.command = WRITE;
		if ((sscanf (&Buffer[1], "%x%x", &address, &value)) != 2)
			goto wrong;
		Request.address = address;
		Request.value = value;
		break;
	case 'E':
	case 'e':
		Request.command = RUN;
		if((sscanf (&Buffer[1], "%d", &value)) != 1)
			goto wrong;
		Request.address = 0x00;
		Request.value = value;
		break;
	case 'D':
	case 'd':
		Request.command = DISPLAY;
		break;
	default:
		goto wrong;
	}
	return TRUE;
 wrong:
	printf("WRONG COMMAND: Please type again.\n");
	return FALSE;
}

int main(){
	pid_t pid;
	int client_fd;
	int server_fd;
	pid = getpid();
	sprintf(Buffer, CLIENT_FIFO_FORM, pid);
	mkfifo(Buffer, 0666);
	server_fd = open(SERVER_FIFO, O_WRONLY);
	Request.pid = pid;
	Request.command = NEW_TASK;
	Request.value = 0;
	write(server_fd, &Request, sizeof(request_t));
	client_fd = open(Buffer, O_RDONLY);
	printf("%s\n", HELP_INFO);
	while (Continue) {
		read(client_fd, Buffer, BUFF_SIZE);
		printf("%s", Buffer);
		while (parse_command() == FALSE)
			;
		write(server_fd, &Request, sizeof(request_t));
	}
	printf("Byebye!\n");
}

#include <my_test.h>

int do_my_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	printf("run do_my_test\r\n");
	for (i = 0; i < argc; i++)
	{
		printf("argv[%d]:%s\r\n", i, argv[i]);		
	}
	return 0;	
}

U_BOOT_CMD(
	my_test, 5, 1, do_my_test,
	"my_test - usage of mytest",
	"help of my_test"
);




deafult:
	@gcc writer.c -o writer
	@file writer
CROSS_COMPILE:
	@aarch64-none-linux-gnu-gcc writer.c -o writer
	@file writer > ../assignments/assignment2/fileresult.txt
clean:
	@rm -f writer
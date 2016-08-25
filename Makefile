CC = gcc

all: astyle clean ulog
	
clean:
	rm -rf ulog
	rm -rf *.o *.orig

astyle:
	find ./ -type f -name '*.[ch]' | xargs -n 1 astyle --style=linux --indent=tab --pad-oper --unpad-paren --align-pointer=name --add-brackets 

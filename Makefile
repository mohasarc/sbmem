all: libsbmemlib.a app create_memory_sb destroy_memory_sb

libsbmemlib.a:  ./src/sbmemlib.c
	gcc -ggdb3 -Wall -c ./src/sbmemlib.c -pthread -lrt -lm
	ar -cvq libsbmemlib.a sbmemlib.o
	ranlib libsbmemlib.a

app: ./src/app.c
	gcc -ggdb3 -Wall -o app ./src/app.c -L. -lsbmemlib -pthread -lrt -lm

create_memory_sb: ./src/create_memory_sb.c
	gcc -ggdb3 -Wall -o create_memory_sb ./src/create_memory_sb.c -L. -lsbmemlib -pthread -lrt -lm

destroy_memory_sb: ./src/destroy_memory_sb.c
	gcc -ggdb3 -Wall -o destroy_memory_sb ./src/destroy_memory_sb.c -L. -lsbmemlib -pthread -lrt -lm

clean: 
	rm -fr *.o *.a *~ a.out  app sbmemlib.o sbmemlib.a libsbmemlib.a  create_memory_sb destroy_memory_sb

# Define a variable of all the .c files so that we don't have to modify this
# file every time a new file is added.
#
# Works by using the wildcard function to find all *.c files in the src
# directory, and then the patsubst function replaces every src/%.c with 
# obj/%.o to produce the desired list of required objects.
#
# www.gnu.org/software/make/manual/make.html#Wildcard-Function
objects = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

headers = $(wildcard src/*.h)

all: binByDate

binByDate: obj bin $(objects) $(headers)
	gcc -g -Wall $(objects) -o bin/binByDate

# Define a single rule for building objects so that the file doesn't need to be
# updated every single time a new class is added.
#
# Implicit variables $< = src/%.c and $@ = obj/%.o
#
# www.gnu.org/software/make/manual/make.html#Pattern-Examples
obj/%.o: src/%.c
	gcc -g -Wall -c $< -o $@

bin:
	mkdir -p ./bin

obj:
	mkdir -p ./obj

clean:
	rm -rf obj
	rm -rf bin

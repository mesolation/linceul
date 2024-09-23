CC = clang
CFLAGS = -Wall -O2 -fobjc-arc

SRC_DIR = src
INC_DIR = include

SRCS = $(SRC_DIR)/rebind.c $(SRC_DIR)/mach_o_utils.c
OBJS = $(SRCS:.c=.o)

LIB_NAME = librebind.a

all: $(LIB_NAME)

$(LIB_NAME): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB_NAME)

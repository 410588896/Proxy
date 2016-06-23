CC 	= g++
CFLAGS	= -g 
LIBS	= -lz -lssl -lcrypto

FOO := $(wildcard ./*.cpp)
BAR := $(basename $(FOO))
OBJ := $(addsuffix .o,$(BAR))
INC := $(wildcard ./*.h)

.cpp.o: 
	$(CC) -c $(CFLAGS) $(LIBS) $<

Proxy:$(OBJ)
	@echo "Compiling......"
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	@echo "Done......"

sinclude $(FOO:.cpp=.d)
%d:%cpp
	@echo "Create depend"
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

release:Proxy
	strip Proxy

clean:
	@echo "Cleaning......"
	rm -f *.o;rm -f *.d;rm -f Proxy;rm -f *~
	@echo "Done......"



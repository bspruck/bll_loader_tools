ALL: bllload bllread bllreset decode_dump

install:
	cp -ia bllload $(LYNX_BIN)
	cp -ia bllread $(LYNX_BIN)
	cp -ia bllreset $(LYNX_BIN)
	cp -ia decode_dump $(LYNX_BIN)

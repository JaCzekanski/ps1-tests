TOPTARGETS = build clean

IMAGES = gpu/bandwidth \
		gpu/benchmark \
		gpu/quad \
		gte-fuzz \
		spu/test \
		timers

$(TOPTARGETS): $(IMAGES)
$(IMAGES):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTAGETS) $(IMAGES)

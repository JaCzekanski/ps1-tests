TOPTARGETS = build clean

IMAGES = gpu/bandwidth \
		gpu/benchmark \
		gpu/quad \
		gpu/transparency \
		gpu/lines \
		gpu/rectangles \
		gpu/texture-overflow \
		gte-fuzz \
		spu/test \
		spu/stereo \
		timers

$(TOPTARGETS): $(IMAGES)
$(IMAGES):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTAGETS) $(IMAGES)

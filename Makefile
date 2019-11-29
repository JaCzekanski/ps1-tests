TOPTARGETS = build clean

IMAGES = gpu/bandwidth \
		gpu/benchmark \
		gpu/quad \
		gpu/transparency \
		gpu/triangle \
		gpu/lines \
		gpu/rectangles \
		gpu/texture-overflow \
		gpu/mask-bit \
		gte-fuzz \
		spu/test \
		spu/stereo \
		timers

$(TOPTARGETS): $(IMAGES)
$(IMAGES):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTAGETS) $(IMAGES)

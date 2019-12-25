TOPTARGETS = build clean

IMAGES = common \
		cpu/code-in-scratchpad \
		dma/otc-test \
		gpu/bandwidth \
		gpu/benchmark \
		gpu/quad \
		gpu/transparency \
		gpu/triangle \
		gpu/lines \
		gpu/rectangles \
		gpu/texture-overflow \
		gpu/mask-bit \
		gpu/gp0-e1 \
		gpu/vram-to-vram-overlap \
		gte-fuzz \
		spu/test \
		spu/stereo \
		timers

$(TOPTARGETS): $(IMAGES)
$(IMAGES):
	@$(MAKE) --no-print-directory -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTAGETS) $(IMAGES)

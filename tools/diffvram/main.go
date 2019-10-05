package main

import (
	"image"
	"image/color"
	"image/png"
	"log"
	"os"
)

func min(a, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}

func abs(a int) int {
	if a < 0 {
		return -a
	} else {
		return a
	}
}

func diff(a, b uint32) uint8 {
	if a == b {
		return 0
	}

	d := abs(int(a) - int(b))

	return uint8(128 + d/2)
}

func main() {
	log.SetFlags(0)

	if len(os.Args) < 3 {
		log.Printf("usage: diffvram img1.png img2.png [diff.png]")
		return
	}

	fimg1, err := os.Open(os.Args[1])
	if err != nil {
		log.Fatal(err)
		return
	}

	fimg2, err := os.Open(os.Args[2])
	if err != nil {
		log.Fatal(err)
		return
	}

	img1, _, err := image.Decode(fimg1)
	if err != nil {
		log.Fatalf("%s: %s.\n", os.Args[1], err)
		return
	}

	img2, _, err := image.Decode(fimg2)
	if err != nil {
		log.Fatalf("%s: %s.\n", os.Args[2], err)
		return
	}

	size1 := img1.Bounds().Size()
	size2 := img2.Bounds().Size()

	if size1 != size2 {
		log.Printf("Size difference: %dx%d vs %dx%d\n", size1.X, size1.Y, size2.X, size2.Y)
	}

	size := image.Point{
		X: min(size1.X, size2.X),
		Y: min(size1.Y, size2.Y),
	}

	diffImage := image.NewRGBA(image.Rectangle{
		Min: image.Point{
			X: 0,
			Y: 0,
		},
		Max: size,
	})

	diffCount := 0
	for y := 0; y < size.Y; y++ {
		for x := 0; x < size.X; x++ {
			p1 := img1.At(x, y)
			p2 := img2.At(x, y)

			if p1 != p2 {
				diffCount++
			}

			r1, g1, b1, a1 := p1.RGBA()
			r2, g2, b2, a2 := p2.RGBA()
			c := color.RGBA{
				R: diff(r1, r2),
				G: diff(g1, g2),
				B: diff(b1, b2),
				A: 255 - diff(a1, a2),
			}

			diffImage.Set(x, y, c)
		}
	}

	if diffCount != 0 {
		log.Printf("Images are different (%d pixels)\n", diffCount)
		diffName := "diff.png"
		if len(os.Args) >= 4 {
			diffName = os.Args[3]
		}

		fdiff, err := os.Create(diffName)
		if err != nil {
			log.Fatal(err)
			return
		}
		err = png.Encode(fdiff, diffImage)
		if err != nil {
			log.Fatalf("%s: %s.\n", diffName, err)
			return
		}

		log.Printf("Diff written to %s", diffName)
		os.Exit(2)
		return
	}

	log.Printf("Images are the same\n")
}

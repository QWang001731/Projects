package main

type sa struct {
	s1 string
	s2 []string
}

func main() {
	s1 := []string{"G", "E"}
	s2 := []string{"N", "B"}
	sa1 := sa{"hello", s1}
	sa2 := sa{"hello", s2}
	var ssa []sa
	ssa = append(ssa, sa1)
	ssa = append(ssa, sa2)
	for _, e := range ssa {
		for _, s := range e.s2 {
			println(s)
		}
	}
	//ssa = append(ssa, sa2)
	/******
		The following code shows you how to work with PNG files in Golang.
	******/

	//Assumes the user specifies a file as the first argument
	/*filePath := os.Args[1]

	//Loads the png image and returns the image or an error
	pngImg, err := png.Load(filePath)

	if err != nil {
		panic(err)
	}

	//Performs a grayscale filtering effect on the image
	pngImg.Grayscale()

	//Saves the image to a new file
	err = pngImg.Save("test_gray.png")

	//Checks to see if there were any errors when saving.
	if err != nil {
		panic(err)
	}*/

}

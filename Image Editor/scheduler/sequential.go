package scheduler

import (
	"encoding/json"
	"fmt"
	"os"
	"proj1/png"
	"strings"
)

type path_effect struct {
	Inpath  string   "json:inpath"
	Outpath string   "json:outpath"
	Effects []string "json:effects"
}
func min(a, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}
func max(a, b int) int {
	if a > b {
		return a
	} else {
		return b
	}
}


func RunSequential(dataDirs string) {
	var eff path_effect
	effectsPathFile := fmt.Sprintf("../data/effects.txt")
	effectsFile, _ := os.Open(effectsPathFile)
	reader := json.NewDecoder(effectsFile)
	effMap := map[string][][]float64{
		"S": {{0, -1, 0},
			{-1, 5, -1},
			{0, -1, 0}},
		"E": {{-1, -1, -1},
			{-1, 8, -1},
			{-1, -1, -1}},
		"B": {{1 / 9.0, 1 / 9, 1 / 9.0},
			{1 / 9.0, 1 / 9.0, 1 / 9.0},
			{1 / 9.0, 1 / 9.0, 1 / 9.0}},
	}
	var pngImg *png.Image
	for i := 0; i < 10; i++ {
		err := reader.Decode(&eff)
		if err != nil {
			println("error in input file")
			os.Exit(1)
		}
		dataDir := strings.Split(dataDirs, "+")
		for _, dir := range dataDir {
			inFile := "../data/in/" + dir + "/" + eff.Inpath
			outFile := "../data/out/" + dir + "_" + strings.Split(eff.Inpath, ".")[0] + "_out.png"
			pngImg, _ = png.Load(inFile)
			for _, e := range eff.Effects {
				if e == "G" {
					pngImg.Grayscale()
					pngImg.In = pngImg.Out
					continue
				}
				png.Conv(pngImg, effMap[e])
				pngImg.In = pngImg.Out
			}
			_ = pngImg.Save(outFile)
		}
	}
}

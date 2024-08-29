package scheduler

import (
	"encoding/json"
	"fmt"
	"proj1/png"
	"proj1/ppsync"
	"strings"
	"sync"
)

func RunParallelSlices(dataDirs string, threadCount int) {
	var tasks Linked_task
	var wg sync.WaitGroup
	lock := ppsync.NewPPLock()
	dataDir := strings.Split(dataDirs, "+")
	effectsPathFile := fmt.Sprintf("../data/effects.txt")
	data, _ := readFile(effectsPathFile)
	lines := strings.Split(string(data), "\n")
	for _, dir := range dataDir {
		for _, line := range lines {
			var task path_effect
			if len(strings.TrimSpace(line)) == 0 {
				continue
			}
			if err := json.Unmarshal([]byte(line), &task); err != nil {
				fmt.Println("Error:", err)
				return
			}
			task.Outpath = "../data/out/" + dir + "_" + strings.Split(task.Inpath, ".")[0] + "_out.png"
			task.Inpath = "../data/in/" + dir + "/" + task.Inpath
			tasks.insert(task)
		}
	}

	for tasks.head != nil {
		task := *tasks.get_and_delete()
		pngImg, _ := png.Load(task.Inpath)
		out_file := task.Outpath
		bounds := pngImg.Bounds
		xmax := bounds.Max.X
		xmin := bounds.Min.X
		stride := (xmax - xmin) / threadCount
		for _, e := range task.Effects {
			for i := 0; i < threadCount; i++ {
				start := max(i*stride-1, 0)
				end := min(start+stride+2, xmax)
				if i == 0 {
					end = end - 1
				}
				wg.Add(1)
				if e == "G" {
					go png.Grays_slice(pngImg, start, end, &wg, lock)
					continue
				}
				go png.Conv_slice(pngImg, e, start, end, &wg, lock)
			}
			wg.Wait()
		}
		pngImg.Save(out_file)
	}
}

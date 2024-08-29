package scheduler

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"proj1/png"
	"proj1/ppsync"
	"strings"
	"sync"
	"time"
)

type Node struct {
	task path_effect
	next *Node
}

type Linked_task struct {
	head *Node
}

func (list *Linked_task) insert(task path_effect) {
	new_node := &Node{task, nil}
	new_node.next = list.head
	list.head = new_node
}

func (list *Linked_task) get_and_delete() *path_effect {
	if list.head == nil {
		fmt.Printf("empty list !")
		return &path_effect{}
	}
	ret := list.head.task
	list.head = list.head.next
	return &ret
}

func readFile(filename string) ([]byte, error) {
	//file, err := os.Open(filename)
	//if err != nil {
	//	return nil, err
	//}
	//defer file.Close() // Ensure file is closed properly

	data, err := ioutil.ReadFile(filename)
	if err != nil {
		return nil, err
	}
	return data, nil
}

func RunParallelFull(dataDirs string, threadCount int) {
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
	img_num := 10 * len(dataDir)
	threadCount = min(threadCount, img_num)
	size := img_num / threadCount
	r := img_num - size*threadCount
	start := time.Now()
	for i := 0; i < r; i++ {
		wg.Add(1)
		go process_image_sub_set(&wg, &tasks, size+1, lock)
	}
	for i := r; i < threadCount; i++ {
		wg.Add(1)
		go process_image_sub_set(&wg, &tasks, size, lock)
	}
	wg.Wait()
	end := time.Since(start).Seconds()
	fmt.Printf("%.6f\n", end)
}

func process_image_sub_set(wg *sync.WaitGroup, tasks *Linked_task, size int, lock *ppsync.PPLock) {
	effMap := map[string][][]float64{
		"S": {{0, -1, 0},
			{-1, 5, -1},
			{0, -1, 0}},
		"E": {{-1, -1, -1},
			{-1, 8, -1},
			{-1, -1, -1}},
		"B": {{1 / 9.0, 1 / 9.0, 1 / 9.0},
			{1 / 9.0, 1 / 9.0, 1 / 9.0},
			{1 / 9.0, 1 / 9.0, 1 / 9.0}},
	}
	for i := 0; i < size; i++ {
		lock.Lock()
		task := tasks.get_and_delete()
		lock.Unlock()
		in_file := task.Inpath
		out_file := task.Outpath
		effect := task.Effects
		pngImg, _ := png.Load(in_file)
		for _, e := range effect {
			if e == "G" {
				pngImg.Grayscale()
				pngImg.In = pngImg.Out
				continue
			}
			png.Conv(pngImg, effMap[e])
			pngImg.In = pngImg.Out
		}
		_ = pngImg.Save(out_file)
	}
	wg.Done()
}

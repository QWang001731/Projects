package main

import (
	"flag"
	"fmt"
	"os"
	"proj1/scheduler"
	"runtime"
	"time"
)

const usage = "usage: editor [-p=string] [-t=int] data_dir" +
	"data_dir: The data directory to use to load the images.\n" +
	"-p=string: Runs a parallel version of the program. The string is either (slices) to run the slice version or (full) to run the full image file version.\n" +
	"-t=int: Specifies as an integer, how many threads to spawn when a parallel version is running. Default= Number of cores of logical cores on the system running the program."

func main() {

	pFlag := flag.String("p", "sequential", "Runs a parallel version of the program. The string is either (slices) to run the slice version or (full) to run the full image file version.")
	tFlag := flag.Int("t", runtime.NumCPU(), "Specifies as an integer, how many threads to spawn when a parallel version is running. Default= Number of cores of logical cores on the system running the program")

	flag.Usage = func() {
		fmt.Fprintln(os.Stderr, usage)
	}
	flag.Parse()

	if len(flag.Args()) != 1 {
		flag.Usage()
		os.Exit(1)
	} else {
		var mode scheduler.SchedulerMode
		if *pFlag == "slices" {
			mode = scheduler.Slices
		} else if *pFlag == "full" {
			mode = scheduler.Full
		} else if *pFlag == "sequential" {
			mode = scheduler.Sequential
		} else {
			fmt.Fprintf(os.Stderr, "Invalid -p flag string")
			os.Exit(1)
		}
		config := scheduler.Config{DataDirs: flag.Arg(0), Mode: mode, ThreadCount: *tFlag}
		start := time.Now()
		scheduler.Schedule(config)
		end := time.Since(start).Seconds()
		fmt.Printf("%.6f\n", end)

	}
}

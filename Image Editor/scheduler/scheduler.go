package scheduler

type Config struct {
	DataDirs    string        //Represents the data directories to use to load the images.
	Mode        SchedulerMode // Represents which scheduler scheme to use
	ThreadCount int           // Runs parallel version with the specified number of threads
}

// Define a custom type for days of the week
type SchedulerMode int

const (
	Sequential SchedulerMode = iota
	Full
	Slices
)

// Run the correct version based on the Mode field of the configuration value
func Schedule(config Config) {
	if config.Mode == Sequential {
		RunSequential(config.DataDirs)
	} else if config.Mode == Full {
		RunParallelFull(config.DataDirs, config.ThreadCount)
	} else if config.Mode == Slices {
		RunParallelSlices(config.DataDirs, config.ThreadCount)
	} else {
		panic("Invalid scheduling scheme given.")
	}
}

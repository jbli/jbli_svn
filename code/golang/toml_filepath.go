package main

import (
	"fmt"
	"github.com/BurntSushi/toml"
	"os"
	"path/filepath"
	"strings"
)

type tomlConfig struct {
	FileList fileList
}

type fileList struct {
	IgnoreFiles []string
	ScanbFiles  []string
}

var scanList []string

func getValidList(scanbFiles, ignoreFiles, validList []string, suffix_str string) error {
	var ignoreList map[string]bool
	ignoreList = make(map[string]bool)
	for _, filename := range ignoreFiles {
		ignoreList[filename] = true
	}

	for _, filename := range scanbFiles {
		finfo, err1 := os.Stat(filename)
		if err1 != nil {
			fmt.Println(filename, "is not exit")
			continue
		}

		if finfo.IsDir() {
			filepath.Walk(filename,
				func(path string, info os.FileInfo, err error) error {
					if err != nil {
						fmt.Printf("ERROR: %v", err)
						return err
					}
					if info.IsDir() {
						return nil
					} else if !ignoreList[path] && strings.HasSuffix(filename, suffix_str) {
						scanList = append(scanList, path)
					}
					return nil
				})
		} else if !ignoreList[filename] && strings.HasSuffix(filename, suffix_str) {
			//fmt.Println(finfo.Name())
			scanList = append(scanList, filename)
		}
	}
	return nil
}

func main() {
	var config tomlConfig
	if _, err := toml.DecodeFile("c1.toml", &config); err != nil {
		fmt.Println(err)
		return
	}

	fmt.Printf("scanbFiles: %v\n", config.FileList.ScanbFiles)
	fmt.Printf("ignoreFiles: %v\n", config.FileList.IgnoreFiles)

	scanList = make([]string, 0, 10)
	getValidList(config.FileList.ScanbFiles, config.FileList.IgnoreFiles, scanList, "twsp")

	for _, filename := range scanList {
		fmt.Println(filename)
	}
}

package main

import (
	"fmt"
	"os"
	"path/filepath"
)

func main() {
	finfo, _ := os.Stat("/tmp")
	list := make([]string, 0, 10)
	if finfo.IsDir() {
		filepath.Walk("/tmp",
			func(path string, info os.FileInfo, err error) error {
				if err != nil {
					fmt.Printf("ERROR: %v", err)
					return err
				}
				if info.IsDir() {
					return nil
				} else {
					list = append(list, path)
				}
				return nil
			}
                        )
	}
	for _, filename := range list {
		fmt.Println(filename)
	}

}

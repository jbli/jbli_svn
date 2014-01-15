/*
 *  遍历一个目录下所有文件
 */

package main

import (
	"fmt"
	"os"
	"path/filepath"
)

func explorer(path string, info os.FileInfo, err error) error {
	if err != nil {
		fmt.Printf("ERROR: %v", err)
		return err
	}
	if info.IsDir() {
		fmt.Printf("dir: %s\n", path)
	} else {
		fmt.Printf("file: %s\n", path)
	}
	return nil
}

func main() {
	finfo, _ := os.Stat("/tmp")
	if finfo.IsDir() {
		filepath.Walk("/tmp", explorer)
	}
}

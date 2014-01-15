package main

import (
	"bufio"
	"bytes"
	"crypto/sha1"
	"encoding/json"
	"fmt"
	"github.com/BurntSushi/toml"
	"io"
	"log"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

type sendCB struct {
	logger *log.Logger
}

type tomlConfig struct {
	FileList  fileList
	GolobConf golobConf
}

type golobConf struct {
	CarbonAddress   string
	SeekJournalDir  string
	Logfile         string
	ResumeFromStart bool
}
type fileList struct {
	IgnoreFiles []string
	ScanbFiles  []string
}

type FileMonitor struct {
	seek               int64
	last_logline_start int64
	last_logline       string
	seekJournalPath    string
	inputfile          string
	resumeFromStart    bool
	carbonAddress      string
	logger             *log.Logger
}

func (fm *FileMonitor) MarshalJSON() ([]byte, error) {
	h := sha1.New()
	io.WriteString(h, fm.last_logline)
	tmp := map[string]interface{}{
		"seek":       fm.seek,
		"last_start": fm.last_logline_start,
		"last_len":   len(fm.last_logline),
		"last_hash":  fmt.Sprintf("%x", h.Sum(nil)),
	}
	fm.logger.Println("seek_pos:", fm.seek, "last_start:", fm.last_logline_start, "last_len", len(fm.last_logline), "last_hash", fmt.Sprintf("%x", h.Sum(nil)))
	return json.Marshal(tmp)
}

func (fm *FileMonitor) updateJournal(bytes_read int64) (ok bool) {
	var seekJournal *os.File
	var file_err error

	if bytes_read == 0 || fm.seekJournalPath == "" {
		return true
	}

	if seekJournal, file_err = os.OpenFile(fm.seekJournalPath,
		os.O_CREATE|os.O_RDWR|os.O_TRUNC,
		0660); file_err != nil {
		fm.logger.Printf("Error opening seek recovery log: %s", file_err.Error())
		return false
	}
	defer seekJournal.Close()

	var filemon_bytes []byte
	//filemon_bytes, _ = json.Marshal(fm)
	filemon_bytes, _ = fm.MarshalJSON()
	if _, file_err = seekJournal.Write(filemon_bytes); file_err != nil {
		fm.logger.Printf("Error writing seek recovery log: %s", file_err.Error())
		return false
	}

	return true
}

func (fm *FileMonitor) UnmarshalJSON(data []byte) (err error) {
	var dec = json.NewDecoder(bytes.NewReader(data))
	var m map[string]interface{}

	defer func() {
		if r := recover(); r != nil {
			fm.logger.Println("Error parsing the journal file")
		}
	}()

	err = dec.Decode(&m)
	if err != nil {
		return nil
	}

	var seek_pos = int64(m["seek"].(float64))
	var last_start = int64(m["last_start"].(float64))
	var last_len = int64(m["last_len"].(float64))
	var last_hash = m["last_hash"].(string)

	//fmt.Println("seek_pos:", seek_pos, "last_start:", last_start, "last_len", last_len, "last_hash", last_hash)
	var fd *os.File
	if fd, err = os.Open(fm.inputfile); err != nil {
		return
	}
	defer fd.Close()
	if _, err = fd.Seek(last_start, 0); err == nil {
		reader := bufio.NewReader(fd)
		buf := make([]byte, last_len)
		_, err := io.ReadAtLeast(reader, buf, int(last_len))
		if err == nil {
			h := sha1.New()
			h.Write(buf)
			tmp := fmt.Sprintf("%x", h.Sum(nil))
			if tmp == last_hash {
				fm.seek = seek_pos
				fm.logger.Printf("Line matches, continuing from byte pos: %d\n", seek_pos)
				return nil
			}
		}
		fm.logger.Printf("Line mismatch.\n")
	}

	if fm.resumeFromStart {
		fm.seek = 0
		fm.logger.Println("Restarting from start of file.")
	} else {
		fm.seek, _ = fd.Seek(0, 2)
		fm.logger.Printf("Restarting from end of file [%d].\n", fm.seek)
	}
	return nil
}

func (fm *FileMonitor) recoverSeekPosition() (err error) {
	// No seekJournalPath means we're not tracking file location.
	if fm.seekJournalPath == "" {
		return
	}

	var seekJournal *os.File
	if seekJournal, err = os.Open(fm.seekJournalPath); err != nil {
		// The inputfile doesn't exist, nothing special to do
		if os.IsNotExist(err) {
			// file doesn't exist, but that's ok, not a real error
			return nil
		} else {
			return
		}
	}
	defer seekJournal.Close()

	var scanner = bufio.NewScanner(seekJournal)
	var tmp string
	for scanner.Scan() {
		tmp = scanner.Text()
	}
	if len(tmp) > 0 {
		//json.Unmarshal([]byte(tmp), &fm)
		fm.UnmarshalJSON([]byte(tmp))
	}

	return
}

func (fm *FileMonitor) processBody() error {
	var IsChanged bool

	f, err := os.Open(fm.inputfile)
	defer f.Close()

	IsChanged = false
	fm.recoverSeekPosition()

	var buffer bytes.Buffer
	f.Seek(fm.seek, 0)
	if nil == err {
		buff := bufio.NewReader(f)
		for {
			line, err := buff.ReadString('\n')
			if err != nil || io.EOF == err {
				break
			}
			if 0 == len(line) || line == "\r\n" || line == "\n" {
				fm.seek += int64(len(line))
				continue
			}
			a := strings.Split(line, " ")
			var str string
			str = fmt.Sprintf("%s %s %s", a[0], a[1], a[2])
			buffer.WriteString(str)
			buffer.WriteString("\n")
			fmt.Println(str)
			fm.last_logline = string(line)
			fm.last_logline_start = fm.seek
			fm.seek += int64(len(line))
			IsChanged = true
		}
	}
	strs := buffer.String()
	fmt.Println(strs)
	sendCarbon(strs, fm.carbonAddress)
	if IsChanged {
		fm.updateJournal(1)
	}
	return nil
}

func initFileMonitor(inputfile, seekJournalPath, carbonAddress string, logger *log.Logger, resumeFromStart bool) *FileMonitor {
	return &FileMonitor{
		seek:               0,
		last_logline_start: 0,
		last_logline:       "",
		seekJournalPath:    seekJournalPath,
		inputfile:          inputfile,
		resumeFromStart:    resumeFromStart,
		logger:             logger,
		carbonAddress:      carbonAddress,
	}

}

func (scb *sendCB) getValidList(scanbFiles, ignoreFiles, validList []string, suffix_str string) ([]string, error) {
	var ignoreList map[string]bool
	ignoreList = make(map[string]bool)
	for _, filename := range ignoreFiles {
		ignoreList[filename] = true
	}

	for _, filename := range scanbFiles {
		finfo, err1 := os.Stat(filename)
		if err1 != nil {
			scb.logger.Println(filename, "is not exit")
			continue
		}

		if finfo.IsDir() {
			filepath.Walk(filename,
				func(path string, info os.FileInfo, err error) error {
					if err != nil {
						scb.logger.Printf("ERROR: %v", err)
						return err
					}
					if info.IsDir() {
						return nil
					} else if !ignoreList[path] && strings.HasSuffix(filename, suffix_str) {
						validList = append(validList, path)
					}
					return nil
				})
		} else if !ignoreList[filename] && strings.HasSuffix(filename, suffix_str) {
			//fmt.Println(finfo.Name())
			validList = append(validList, filename)
		}
	}
	return validList, nil
}
func (scb *sendCB) getLastFilename(filename string) (string, error) {
	finfo, err := os.Stat(filename)
	if err != nil {
		scb.logger.Println(filename, "is not exit")
		return "", err
	}
	return finfo.Name(), nil
}
func (scb *sendCB) getSeekJournalPath(seekJournalDir, inputfile string) string {
	lastFilename, _ := scb.getLastFilename(inputfile)
	filepath.Join(seekJournalDir, lastFilename)
	return filepath.Join(seekJournalDir, lastFilename)
}

func main() {
	var config tomlConfig

	if _, err := toml.DecodeFile("c1.toml", &config); err != nil {
		fmt.Println(err)
		return
	}

	fmt.Printf("scanbFiles: %v\n", config.FileList.ScanbFiles)
	fmt.Printf("ignoreFiles: %v\n", config.FileList.IgnoreFiles)

	seekJournalDir := config.GolobConf.SeekJournalDir
	logfile := config.GolobConf.Logfile
	resumeFromStart := config.GolobConf.ResumeFromStart
	carbonAddress := config.GolobConf.CarbonAddress
	fmt.Println(seekJournalDir)

	errlog, err := os.OpenFile(logfile, os.O_RDWR|os.O_CREATE, 0)
	if err != nil {
		fmt.Printf("%s\r\n", err.Error())
		os.Exit(-1)
	}
	defer errlog.Close()
	logger := log.New(errlog, "\r\n", log.Ldate|log.Ltime|log.Llongfile)

	scb := &sendCB{
		logger: logger,
	}
	if dirInfo, err := os.Stat(seekJournalDir); err != nil {
		if os.IsNotExist(err) {
			if err = os.MkdirAll(seekJournalDir, 0775); err != nil {
				fmt.Println(fmt.Sprintf("Error creating seek journal folder %s: %s",
					seekJournalDir, err))
				return
			}
		} else {
			fmt.Println(fmt.Sprintf("Error accessing seek journal folder %s: %s",
				seekJournalDir, err))
			return
		}
	} else if !dirInfo.IsDir() {
		fmt.Println("%s doesn't appear to be a directory", seekJournalDir)
		return
	}

	scanList := make([]string, 0, 10)
	vlist, _ := scb.getValidList(config.FileList.ScanbFiles, config.FileList.IgnoreFiles, scanList, "twsp")
	for _, filename := range vlist {
		seekJournalPath := scb.getSeekJournalPath(seekJournalDir, filename)
		fm := initFileMonitor(filename, seekJournalPath, carbonAddress, scb.logger, resumeFromStart)
		fm.processBody()
		//fmt.Println(filename)
	}
	return
}

func sendCarbon(str string, carbonAddress string) {
	var e error
	lines := strings.Split(strings.Trim(str, " \n"), "\n")

	clean_statmetrics := make([]string, len(lines))
	index := 0
	for _, line := range lines {
		// `fields` should be "<name> <value> <timestamp>"
		fields := strings.Fields(line)
		if len(fields) != 3 {
			fmt.Errorf("malformed statmetric line: '%s'", line)
			continue
		}

		if _, e = strconv.ParseUint(fields[2], 0, 32); e != nil {
			fmt.Errorf("parsing time: %s", e)
			continue
		}
		if _, e = strconv.ParseFloat(fields[1], 64); e != nil {
			fmt.Errorf("parsing value '%s': %s", fields[1], e)
			continue
		}
		clean_statmetrics[index] = line
		index += 1
	}
	clean_statmetrics = clean_statmetrics[:index]

	conn, err := net.Dial("tcp", carbonAddress)
	if err != nil {
		fmt.Errorf("Dial failed: %s",
			err.Error())
		return
	}
	defer conn.Close()

	// Stuff each parseable statmetric into a bytebuffer
	var buffer bytes.Buffer
	for i := 0; i < len(clean_statmetrics); i++ {
		buffer.WriteString(clean_statmetrics[i] + "\n")
	}

	_, err = conn.Write(buffer.Bytes())
	if err != nil {
		fmt.Errorf("Write to server failed: %s",
			err.Error())
	}
}

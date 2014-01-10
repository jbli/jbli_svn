package main

import (
    "bytes"
    "fmt"
    "net"
    "strconv"
    "strings"
)

func sendCarbon(str string) {
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

    conn, err := net.Dial("tcp", "192.168.1.46:2003")
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

func main() {
    //str := "collectd_tmp_bj0-45.traffic.rx 12.40 1389261421\ncollectd_tmp_bj0-45.traffic.tx 20.40 1389261421"
    str := "collectd_tmp_bj0-45.traffic.rx 1.17 1389270721\ncollectd_tmp_bj0-45.traffic.tx 2.60 1389270721\n"
    sendCarbon(str)
}


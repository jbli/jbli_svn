func getHostname() (string,error){
        hostname, err := os.Hostname()
        if err != nil {
                return "", err
        }
        shortHostname := strings.Split(hostname, ".")[0]
        return shortHostname, nil
}
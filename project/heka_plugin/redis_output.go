package examples

import (
        "fmt"
        "github.com/adeven/redismq"
        "github.com/mozilla-services/heka/pipeline"
)

type RedisMQOutputConfig struct {
        Address string `toml:"address"`
}

type RedisMQOutput struct {
        conf    *RedisMQOutputConfig
        //rdqueue *redismq.BufferedQueue
        rdqueue *redismq.Queue
}

func (ro *RedisMQOutput) ConfigStruct() interface{} {
        return &RedisMQOutputConfig{"192.168.1.44"}
}

func (ro *RedisMQOutput) Init(config interface{}) error {
        ro.conf = config.(*RedisMQOutputConfig)
        //var err error
        //ro.rdqueue, err = redismq.SelectBufferedQueue(ro.conf.Address, "6379", "", 9, "example", 200)
        //ro.rdqueue, err = redismq.SelectQueue(ro.conf.Address, "6379", "", 9, "clicks")
        //if err != nil {
           //server := redismq.NewServer(ro.conf.Address, "6379", "", 9, "9999")
           //server.Start()
           //ro.rdqueue = redismq.CreateBufferedQueue(ro.conf.Address, "6379", "", 9, "example", 200)
           //err := ro.rdqueue.Start()
           //if err != nil {
           //     panic(err)
           //}
           //ro.rdqueue = redismq.CreateQueue(ro.conf.Address, "6379", "", 9, "clicks")
        //}
        ro.rdqueue = redismq.CreateQueue(ro.conf.Address, "6379", "", 9, "clicks")
        return nil
}

func (ro *RedisMQOutput) Run(or pipeline.OutputRunner, h pipeline.PluginHelper) error {
        var outgoing string
        for pack := range or.InChan() {
                outgoing = fmt.Sprintf("%s", pack.Message.GetPayload())
                ro.rdqueue.Put(outgoing)
                pack.Recycle()
        }

        return nil
}

func init() {
        pipeline.RegisterPlugin("RedisMQOutput", func() interface{} {
                return new(RedisMQOutput)
        })
}

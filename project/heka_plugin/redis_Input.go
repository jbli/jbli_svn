package examples

import (
        "fmt"
        "github.com/adeven/redismq"
        "github.com/mozilla-services/heka/pipeline"
        "time"
)

type RedisMQInputConfig struct {
        Address string `toml:"address"`
        Decoder string `toml:"decoder"`
        StatInterval int `toml:"stat_interval"`
}

type RedisMQInput struct {
        conf            *RedisMQInputConfig
        //rdqueue         *redismq.BufferedQueue
        rdqueue         *redismq.Queue
        rdconsumer      *redismq.Consumer
        stopChan        chan bool
        statInterval     time.Duration
}

func (ri *RedisMQInput) ConfigStruct() interface{} {
        return &RedisMQInputConfig{"192.168.1.44", "",500}
}

func (ri *RedisMQInput) Init(config interface{}) error {
        ri.conf = config.(*RedisMQInputConfig)
        statInterval := ri.conf.StatInterval
        ri.statInterval = time.Millisecond * time.Duration(statInterval)
        ri.stopChan = make(chan bool)
        var err error
        //ri.rdqueue, err = redismq.SelectQueue(ri.conf.Address, "6379", "", 9, "clicks")
        //if err != nil {
        //        ri.rdqueue = redismq.CreateQueue(ri.conf.Address, "6379", "", 9, "clicks")
        //}
        ri.rdqueue = redismq.CreateQueue(ri.conf.Address, "6379", "", 9, "clicks")
        ri.rdconsumer, err = ri.rdqueue.AddConsumer("testconsumer")
        if err != nil {
                panic(err)
        }
        return nil
}

func (ri *RedisMQInput) Run(ir pipeline.InputRunner, h pipeline.PluginHelper) error {
        // Get the InputRunner's chan to receive empty PipelinePacks
        packs := ir.InChan()

        var decoding chan<- *pipeline.PipelinePack
        if ri.conf.Decoder != "" {
                // Fetch specified decoder
                decoder, ok :=  h.DecoderRunner(ri.conf.Decoder)
                if !ok {
                        err := fmt.Errorf("Could not find decoder", ri.conf.Decoder)
                        return err
                }

                // Get the decoder's receiving chan
                decoding = decoder.InChan()
        }

        var pack *pipeline.PipelinePack
        //var p []*redismq.Package
        var p *redismq.Package
        var count int
        var b []byte
        var err error
        
        for {
                p, err = ri.rdconsumer.Get()
                if err != nil {
                        ir.LogError(err)
                        continue
                }
                err = p.Ack()
                if err != nil {
                        ir.LogError(err)
                }
                b = []byte(p.Payload)
                // Grab an empty PipelinePack from the InputRunner
                pack = <-packs

                // Trim the excess empty bytes
                count = len(b)
                pack.MsgBytes = pack.MsgBytes[:count]

                // Copy ws bytes into pack's bytes
                copy(pack.MsgBytes, b)

                if decoding != nil {
                        // Send pack onto decoder
                        decoding <- pack
                } else {
                        // Send pack into Heka pipeline
                        ir.Inject(pack)
                }
        }
/*
        checkStat := time.Tick(ri.statInterval)
        ok := true
        for ok {
            select {
		case _, ok = <-ri.stopChan:
			break
		case <-checkStat:
                    p, err = ri.rdconsumer.MultiGet(500)
                    if err != nil {
                        ir.LogError(err)
                        continue
                    }
                    err = p[len(p)-1].MultiAck()
                    if err != nil {
                        ir.LogError(err)
                    }
                    for _, v := range p {
                      b = []byte(v.Payload)
                      // Grab an empty PipelinePack from the InputRunner
                      pack = <-packs

                      // Trim the excess empty bytes
                      count = len(b)
                      pack.MsgBytes = pack.MsgBytes[:count]

                      // Copy ws bytes into pack's bytes
                      copy(pack.MsgBytes, b)

                      if decoding != nil {
                        // Send pack onto decoder
                        decoding <- pack
                      } else {
                        // Send pack into Heka pipeline
                        ir.Inject(pack)
                      }
                    }
                }
        }
*/
        return nil
}

func (ri *RedisMQInput) Stop() {
       close(ri.stopChan)
}

func init() {
        pipeline.RegisterPlugin("RedisMQInput", func() interface{} {
                return new(RedisMQInput)
        })
}

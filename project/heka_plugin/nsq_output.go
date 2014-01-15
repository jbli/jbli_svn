package examples

import (
	"fmt"
	nsq "github.com/bitly/go-nsq"
	"github.com/mozilla-services/heka/client"
	"github.com/mozilla-services/heka/message"
	"github.com/mozilla-services/heka/pipeline"
)

type NsqOutputConfig struct {
	Address   string `toml:"address"`
	Topic     string `toml:"topic"`
	Serialize bool   `toml:"serialize"`
}

type NsqOutput struct {
	conf      *NsqOutputConfig
	nsqwriter *nsq.Writer
}

func (no *NsqOutput) ConfigStruct() interface{} {
	return &NsqOutputConfig{
		Address:   "192.168.1.44:4160",
		Topic:     "test",
		Serialize: true,
	}
}

func (no *NsqOutput) Init(config interface{}) error {
	no.conf = config.(*NsqOutputConfig)
	no.nsqwriter = nsq.NewWriter(no.conf.Address)
	return nil
}

func (no *NsqOutput) Run(or pipeline.OutputRunner, h pipeline.PluginHelper) (err error) {
	var (
		encoder client.Encoder
		msg     *message.Message
		msgBody []byte = make([]byte, 0, 1024)
		pack    *pipeline.PipelinePack
	)

	conf := no.conf
	encoder = client.NewProtobufEncoder(nil)

	for pack = range or.InChan() {
		if conf.Serialize {
			msg = pack.Message
			if err = encoder.EncodeMessageStream(msg, &msgBody); err != nil {
				or.LogError(err)
				err = nil
				pack.Recycle()
				continue
			}
			//err := no.nsqwriter.PublishAsync(conf.Topic, []byte(pack.Message.GetPayload()), nil)
			//err = no.nsqwriter.PublishAsync(conf.Topic, msgBody, nil)
			_,_,err = no.nsqwriter.Publish(conf.Topic, msgBody)
			if err != nil {
				or.LogError(fmt.Errorf("error in writer.PublishAsync"))
			}
			msgBody = msgBody[:0]
		} else {
			err = no.nsqwriter.PublishAsync(conf.Topic, []byte(pack.Message.GetPayload()), nil)
			if err != nil {
				or.LogError(fmt.Errorf("error in writer.PublishAsync"))
			}
		}
		pack.Recycle()
	}

	return
}

func init() {
	pipeline.RegisterPlugin("NsqOutput", func() interface{} {
		return new(NsqOutput)
	})
}

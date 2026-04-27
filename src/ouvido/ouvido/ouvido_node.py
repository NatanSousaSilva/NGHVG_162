import rclpy
from rclpy.node import Node
from std_msgs.msg import String

import sounddevice as sd
import json
from vosk import Model, KaldiRecognizer
import os


class Ouvido_Node(Node):
    def __init__(self):
        super().__init__("ouvido_node")
        self.get_logger().info("ouvido inicializado")

        self._pub_llm = self.create_publisher(String, "ouvido/llm", 10)

        self._model_path = os.path.expanduser("~/iracema/vosk_models/vosk-model-small-pt-0.3")
        self._model = Model(self._model_path)
        self._recognizer = KaldiRecognizer(self._model, 16000)

        self._stream = sd.RawInputStream(
            samplerate=16000,
            blocksize=8000,
            dtype='int16',
            channels=1,
            callback=self.captar_audio
        )
        self._stream.start()

    def captar_audio(self, indata, frames, time, status):
        if status:
            self.get_logger().warning(str(status))

        audio_bytes = bytes(indata)

        if self._recognizer.AcceptWaveform(audio_bytes):
            result = json.loads(self._recognizer.Result())
            texto = result.get("text", "").lower().strip()

            if texto:
                msg = String()
                msg.data = texto
                self._pub_llm.publish(msg)

                self.get_logger().info(f"Publicado LLM: {texto}")


def main(args=None):
    rclpy.init(args=args)
    node = Ouvido_Node()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node._stream.stop()
        node._stream.close()
        node.destroy_node()
        rclpy.shutdown()

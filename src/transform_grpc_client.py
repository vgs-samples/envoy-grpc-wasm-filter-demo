import logging

import grpc
import transform_pb2
import transform_pb2_grpc


def run():
    with grpc.insecure_channel('localhost:8888') as channel:
        stub = transform_pb2_grpc.TransformStub(channel)
        request = transform_pb2.HeaderRequest(path='/my-path')
        header = request.headers.add()
        header.key = "foo"
        header.value = "bar"
        header = request.headers.add()
        header.key = "x-change-me"
        header.value = "spam"
        response = stub.TransformHeader(request)
        print(response)


if __name__ == '__main__':
    logging.basicConfig()
    run()


from concurrent import futures
import logging

import grpc
import transform_pb2
import transform_pb2_grpc


class Transformer(transform_pb2_grpc.TransformServicer):

    def TransformHeader(self, request, context):
        logger = logging.getLogger(__name__)
        logger.info("Get header request %s", request)
        response = transform_pb2.HeaderResponse(path=request.path)
        for item in request.headers:
            if item.key == "x-change-me":
                header = response.headers.add()
                header.key = item.key
                header.value = f"modified {item.value}"
                logger.info("Modify header %s => %s", item.key, header.value)
        header = response.headers.add()
        header.key = "x-new-header"
        header.value = "whatsup"
        return response

    def TransformBody(self, request, context):
        logger = logging.getLogger(__name__)
        logger.info("Get body request %s", request)
        return transform_pb2.BodyResponse(content=request.content.replace("foo", "eggs"))


def serve():
    logger = logging.getLogger(__name__)
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    transform_pb2_grpc.add_TransformServicer_to_server(Transformer(), server)
    logger.info("Serving at :8888")
    server.add_insecure_port('0.0.0.0:8888')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    serve()

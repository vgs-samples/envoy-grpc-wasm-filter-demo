from concurrent import futures
import logging

import grpc
import transform_pb2
import transform_pb2_grpc


class Transformer(transform_pb2_grpc.TransformServicer):

    def TransformHeader(self, request, context):
        logger = logging.getLogger(__name__)
        logger.info("Get request %s", request)
        return transform_pb2.HeaderResponse(path=request.path)


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

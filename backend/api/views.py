from rest_framework import status
from rest_framework.response import Response
from rest_framework.views import APIView

from .models import LuminosidadeReading
from .serializers import LuminosidadeReadingSerializer


class StatusView(APIView):
    def get(self, request, format=None):
        return Response({"status": "online"}, status=status.HTTP_200_OK)


class LuminosidadeView(APIView):
    def get(self, request, format=None):
        try:
            latest_reading = LuminosidadeReading.objects.latest('timestamp')
            serializer = LuminosidadeReadingSerializer(latest_reading)
            return Response(serializer.data, status=status.HTTP_200_OK)
        except LuminosidadeReading.DoesNotExist:
            return Response({"erro": "Nenhuma leitura"}, status=status.HTTP_404_NOT_FOUND)

    def post(self, request, format=None):
        serializer = LuminosidadeReadingSerializer(data=request.data)
        if serializer.is_valid():
            serializer.save()
            return Response(serializer.data, status=status.HTTP_201_CREATED)
        return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)


class ControleView(APIView):
    def post(self, request, format=None):
        print("Comando manual recebido:", request.data)
        return Response(
            {"sucesso": True, "modo_definido": request.data.get('modo')},
            status=status.HTTP_200_OK,
        )


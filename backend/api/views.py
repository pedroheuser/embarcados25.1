import time

from rest_framework import status
from rest_framework.response import Response
from rest_framework.views import APIView

from .models import LuminosidadeReading
from .serializers import LuminosidadeReadingSerializer

# Armazena o último comando manual recebido (em memória)
# Formato: {"modo": "manual", "cor": {"r": int, "g": int, "b": int}, "timestamp": float}
ultimo_comando_manual = None


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
    def get(self, request, format=None):
        """
        Retorna o último comando manual pendente.
        Usado pelo ESP8266 para polling de comandos.
        """
        global ultimo_comando_manual
        
        if ultimo_comando_manual is None:
            return Response({"modo": "auto"}, status=status.HTTP_200_OK)
        
        # Retornar o comando armazenado
        return Response(ultimo_comando_manual, status=status.HTTP_200_OK)

    def post(self, request, format=None):
        """
        Recebe comando manual do aplicativo móvel.
        Armazena o comando para ser recuperado pelo ESP8266 via GET.
        """
        global ultimo_comando_manual
        
        print("Comando recebido:", request.data)
        
        # Armazenar comando para o ESP8266 buscar
        modo = request.data.get('modo', 'auto')
        cor = request.data.get('cor', {})
        
        if modo == 'manual' and cor:
            # Armazenar comando manual
            ultimo_comando_manual = {
                "modo": "manual",
                "cor": {
                    "r": cor.get('r', 0),
                    "g": cor.get('g', 0),
                    "b": cor.get('b', 0),
                },
                "timestamp": time.time(),
            }
        elif modo == 'auto':
            # Limpar comando manual para retornar ao modo automático
            ultimo_comando_manual = None
        
        return Response(
            {"sucesso": True, "modo_definido": modo},
            status=status.HTTP_200_OK,
        )


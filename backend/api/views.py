# Conteúdo completo para: api/views.py

from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework import status

# Importamos os novos models e serializers
from .models import LuminosidadeReading, ControlState
from .serializers import LuminosidadeReadingSerializer, ControlStateSerializer

#
# View 1: Status (Não muda)
#
class StatusView(APIView):
    def get(self, request, format=None):
        return Response({"status": "online"}, status=status.HTTP_200_OK)

#
# View 2: Luminosidade (Não muda)
#
class LuminosidadeView(APIView):
    """
    GET: Retorna a última leitura de luminosidade.
    POST: (Chamado pelo Wemos) Recebe uma nova leitura.
    """
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

#
# View 3: Controle (NOVA VERSÃO)
#
class ControleView(APIView):
    """
    Controla a "memória" de estado.
    GET: (Chamado pelo Wemos) Retorna o último comando de controle.
    POST: (Chamado pelo App) Define um novo comando de controle.
    """
    
    def get_state_object(self):
        # Usa get_or_create para garantir que a linha de estado (id=1)
        # sempre exista no banco de dados.
        obj, created = ControlState.objects.get_or_create(id=1, defaults={
            'modo': 'auto',
            'r': 0, 'g': 0, 'b': 0
        })
        return obj

    def get(self, request, format=None):
        """ O Wemos faz GET aqui a cada 2 segundos """
        state = self.get_state_object()
        
        # Montamos o JSON no formato que o Wemos espera
        data_to_send = {
            'modo': state.modo,
            'cor': {
                'r': state.r,
                'g': state.g,
                'b': state.b
            }
        }
        
        # Validamos os dados (boa prática) e retornamos
        serializer = ControlStateSerializer(data=data_to_send)
        if serializer.is_valid():
            return Response(serializer.data, status=status.HTTP_200_OK)
        # Se isso falhar, é um erro interno do servidor
        return Response(serializer.errors, status=status.HTTP_500_INTERNAL_SERVER_ERROR)


    def post(self, request, format=None):
        """ O App React Native faz POST aqui """
        state = self.get_state_object()
        
        # Valida os dados recebidos (ex: {"modo": "manual", "cor": {...}})
        serializer = ControlStateSerializer(data=request.data)
        
        if serializer.is_valid():
            # Dados validados
            data = serializer.validated_data
            cor_data = data.get('cor')

            # Atualiza o objeto "memória" no banco de dados
            state.modo = data.get('modo')
            
            # Se modo for manual e cor foi fornecida, atualizar RGB
            # Se modo for auto, manter os valores atuais (ou zerar)
            if state.modo == 'manual' and cor_data:
                state.r = cor_data.get('r', 0)
                state.g = cor_data.get('g', 0)
                state.b = cor_data.get('b', 0)
            elif state.modo == 'auto':
                # Em modo auto, podemos zerar ou manter os valores
                # (o ESP8266 não vai usar esses valores no modo auto)
                pass
            
            state.save()
            
            # Retorna o novo estado salvo (montar resposta completa)
            response_data = {
                'modo': state.modo,
                'cor': {
                    'r': state.r,
                    'g': state.g,
                    'b': state.b
                }
            }
            return Response(response_data, status=status.HTTP_200_OK)
        
        # Se os dados do App forem inválidos (ex: "modo": "teste")
        return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)
from django.urls import path

from .views import ControleView, LuminosidadeView, StatusView


urlpatterns = [
    path('status/', StatusView.as_view(), name='status'),
    path('luminosidade/', LuminosidadeView.as_view(), name='luminosidade'),
    path('controle/', ControleView.as_view(), name='controle'),
]


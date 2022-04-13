# winiarze
## Algorytm 
Winiarze uproszczone
Krzysztof Krzyżaniak 145334
Michał Kwarta 145192

Studenci zawsze biorą całe wino, winiarze więc produkują po prostu "jedną jednostkę" wina. Winiarz może zdecydować, komu dać wino. 
Parametry:
S – liczba studentów
W – liczba winiarzy
B – liczba bezpiecznych miejsc

Zasoby lokalne:
clock – aktualna wartość zegara Lamporta
winiarze_announcements – tablica ogłoszeń winiarzy
studenty_requests  – posortowana po clock, pid tablica żądań wina
bezpieczne_miejsce_status – (tylko dla winiarzy) tablica o rozmiarze B statusów zajęcia bezpiecznych miejsc
bezpieczne_miejsce_requests – posortowana tablica żądań o bezpieczne miejsce po clock oraz pid

Proces winiarza:
Zegary clock procesów są inicjalizowane na wartość 0, statusy zajęcia bezpiecznego miejsca są ustawiane na EMPTY.
Winiarz odczekuje losowy czas i po jego upłynięciu produkuje 1 beczkę wina.
Winiarz wysyła do wszystkich winiarzy (łącznie ze sobą, wtedy wrzuca do swojej kolejki) żądanie rezerwacji jednego bezpiecznego miejsca (wraz ze swoim zegarem oraz pid).
Jeżeli żądanie winiarza jest na szczycie kolejki bezpieczne_miejsce_requests i otrzymał potwierdzenie od wszystkich innych winiarzy oraz istnieje minimum jedno wolne bezpieczne miejsce w kolejce bezpieczne_miejsce_requests, winiarz wysyła do innych winiarzy informację o zajęciu jednego z bezpiecznych miejsc oraz wysyła ogłoszenie o wyprodukowanym winie do studentów.
Obsługa wiadomości:
Jeżeli dostaniemy żądanie o bezpieczne miejsce, dołączamy żądanie do kolejki bezpieczne_miejsce_requests (i ją sortujemy po clock, pid). Aktualizujemy zegar lamporta (clock z żądania + 1) oraz wysyłamy potwierdzenie odbioru do nadawcy żądania wraz z wartością zegara lamporta.
Jeżeli dostaniemy komunikat o zajęciu bezpiecznego miejsca, aktualizujemy bezpieczne_miejsce_status oraz usuwamy żądanie zajmującego z kolejki  bezpieczne_miejsce_requests.
Jeżeli dostaniemy komunikat o zwolnieniu bezpiecznego miejsca, aktualizujemy bezpieczne_miejsce_status.
Jeżeli dostaniemy komunikat od studenta o przejęciu beczki wina, (zakładając, że sama czynność utylizacji trwa 0) wysyłamy komunikat o zwolnieniu bezpiecznego miejsca do innych winiarzy i przechodzimy do kroku 2.
W przypadku wystąpienia scenariusza 6.1, 6.2 lub 6.3, wykonujemy ponownie (tylko) krok 4 i oczekuje na przejęcie beczki przez studenta.
Proces studenta:
Zegary clock procesów są inicjalizowane na wartość 0, wszystkie kolejki są puste.
Student wysyła do wszystkich studentów (łącznie ze sobą, wtedy wrzuca do swojej kolejki studenty_requests) żądanie (wraz ze swoim zegarem oraz pid) utylizacji jednej beczki wina.
Jeżeli żądanie studenta jest na szczycie kolejki studenty_requests i otrzymał potwierdzenie od wszystkich innych studentów oraz istnieje minimum jedna dostępna beczka wina w winiarze_announcements, student wysyła do winiarza oraz pozostałych studentów informację o przejęciu beczki wina.
Student po utylizacji beczki wina jest losowy czas niezdolny do dalszej pracy.
Obsługa wiadomości:
Jeżeli dostaniemy żądanie o beczkę wina, dołączamy żądanie do kolejki studenty_requests (i ją sortujemy po clock, pid). Aktualizujemy zegar lamporta (clock z żądania + 1) oraz wysyłamy potwierdzenie odbioru do nadawcy żądania wraz z wartością zegara lamporta.
Jeżeli otrzymamy informację o przejęciu beczki wina, usuwamy żądanie nadawcy z kolejki studenty_requests oraz zmieniamy status winiarza w kolejce winiarze_announcements na EMPTY.
Jeżeli otrzymamy ogłoszenia o beczce wina od winiarza, uzupełniamy odpowiedni wpis w winiarze_announcements.
W przypadku wystąpienia scenariusza 5.1, 5.2 lub 5.3 skaczemy do kroku nr 3.

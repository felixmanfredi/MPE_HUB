import { Component, Input, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { SonyData } from './sony.model';
import { interval } from 'rxjs';
import { CommonModule } from '@angular/common';
import { HttpClient, HttpClientModule } from '@angular/common/http';
//import { SonyService } from './sony.service';
import { environment } from '../environments/environment';
import { TickService } from '../services/tick.service'; // <-- IMPORTA IL SERVIZIO
import { Subscription } from 'rxjs';


@Component({
  selector: 'app-sony',
  standalone: true,
  //imports: [CommonModule, HttpClientModule, MdbFormsModule, MatCardModule, MatIconModule, MatButtonModule],
  imports: [FormsModule, CommonModule, HttpClientModule],
  templateUrl: './sony.component.html',
  styleUrls: ['./sony.component.css']
})
export class SonyComponent implements OnInit {
  sonyData: SonyData | null = null; // Utilizza il modello
  loading = true;
  error: string | null = null;
  isUpdating = false;
  private apiUrl = environment.apiUrl;
  isPhoto = false;
  private componentId = environment.sonyID; // ID UNIVOCO per questo componente
  private tickSub!: Subscription;

  constructor(private http: HttpClient, private tickService: TickService) { }

  ngOnInit(): void {
    this.tickSub = this.tickService.tick$.subscribe(tick => {
      // Esegui la richiesta solo se il tick è allineato con il tuo ID
      if (tick % 4 === this.componentId) {
        this.loadSonyData();
      }
    });
  }

  ngOnDestroy(): void {
    if (this.tickSub) {
      this.tickSub.unsubscribe();
    }
  }

  loadSonyData(): void {
    this.loading = true;
    this.error = null;
    this.http.get<SonyData>(`${this.apiUrl}/api/sony`).subscribe({
      next: (data) => {
        this.sonyData = data;
        this.loading = false;
      },
      error: (err) => {
        this.error = 'Errore nel caricamento dei dati.';
        console.error(err);
        this.loading = false;
      }
    });
    //console.log(this.sonyData);
  }

  togglePower(): void {
    if (this.isUpdating) return;
    this.isUpdating = true;
    if (this.sonyData) {
      //this.http.post<SonyData>(`${this.apiUrl}/sony`, { state: !this.sonyData.isPowered }).subscribe({
      this.http.post<SonyData>(`${this.apiUrl}/api/sony/power`, { isPowered: this.sonyData.isPowered }).subscribe({
        next: (data) => {
          this.sonyData = data;
          this.isUpdating = false;
        },
        error: (err) => {
          console.error('Errore nell\'aggiornamento dello stato di alimentazione.', err);
          this.isUpdating = false;
        }
      });
    }
  }



  triggerPhoto() {
    if (this.isUpdating) return;
  
    this.isUpdating = true;
    this.isPhoto = true;
  
    // Invia POST al server
    this.http.post('/api/sony/takepicture', { takePicture: 250 })
      .subscribe({
        next: () => {
          // Attendi la durata dell'impulso prima di spegnere animazione
          setTimeout(() => {
            this.isPhoto = false;
            this.isUpdating = false;
          }, 1000);
        },
        error: (err) => {
          console.error('Errore POST:', err);
          this.isPhoto = false;
          this.isUpdating = false;
        }
      });
  }




  // TakePicture(): void {
  //   if (this.isUpdating) return;
  //   this.isUpdating = true;
  //   if (this.sonyData) {
  //     //this.http.post<SonyData>(`${this.apiUrl}/sony`, { state: !this.sonyData.isPowered }).subscribe({
  //       //this.http.post<SonyData>(`${this.apiUrl}/api/prova`, { takePicture: this.sonyData.takePicture }, { headers }).subscribe({
  //         //this.http.post<SonyData>(`${this.apiUrl}/api/sony/takepicture`, { takePicture: this.sonyData.takePicture }).subscribe({
  //     this.http.post<SonyData>(`${this.apiUrl}/api/sony/takepicture`, { takePicture: 150 }).subscribe({    
  //       next: (data) => {
  //         this.sonyData = data;
  //         this.isUpdating = false;
  //       },
  //       error: (err) => {
  //         console.error('Errore nell\'acquisizione dell\'immagine.', err);
  //         this.isUpdating = false;
  //       }
  //     });
  //   }
  // }

    getStateColor(): string {
      if (this.sonyData) {
      switch(this.sonyData.state) {
        case 'normal':
          return 'green';
        case 'idle':
          return 'blue';
        case 'watning':
          return 'orange';
        case 'error':
          return 'red';
        default:
          return 'gray';
      }
    }
    return '';
  }

  getStatusColor(isConnected: boolean): string {
    if (isConnected) {
      return '#4CAF50'; // Verde
    } else {
      return '#F44336'; // Rosso
    }
  }

  // getStateClass(): string {
  //   if (this.sonyData) {
  //     switch (this.sonyData.state) {
  //       case 'warning':
  //         return 'warning';
  //       case 'error':
  //         return 'error';
  //       case 'idle':
  //         return 'idle';
  //       case 'normal':
  //       default:
  //         return 'normal';
  //     }
  //   }
  //   return '';
  // }


  
}

import { Component, Input, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { VideoencData } from './videoenc.model';
import { interval } from 'rxjs';
import { CommonModule } from '@angular/common';
import { HttpClient, HttpClientModule } from '@angular/common/http';
//import { VideoencService } from './videoenc.service';
import { environment } from '../environments/environment';
import { TickService } from '../services/tick.service'; // <-- IMPORTA IL SERVIZIO
import { Subscription } from 'rxjs';

@Component({
  selector: 'app-videoenc',
  standalone: true,
  //imports: [CommonModule, HttpClientModule, MdbFormsModule, MatCardModule, MatIconModule, MatButtonModule],
  imports: [FormsModule, CommonModule, HttpClientModule],
  templateUrl: './videoenc.component.html',
  styleUrls: ['./videoenc.component.css']
})
export class VideoencComponent implements OnInit {
  videoencData: VideoencData | null = null; // Utilizza il modello
  loading = true;
  error: string | null = null;
  isUpdating = false;
  private apiUrl = environment.apiUrl;
  private componentId = environment.videoencID; // ID UNIVOCO per questo componente
  private tickSub!: Subscription;

  constructor(private http: HttpClient, private tickService: TickService) { }

  ngOnInit(): void {
    this.tickSub = this.tickService.tick$.subscribe(tick => {
      // Esegui la richiesta solo se il tick è allineato con il tuo ID
      if (tick % 4 === this.componentId) {
        this.loadVideoencData();
      }
    });
  }

  ngOnDestroy(): void {
    if (this.tickSub) {
      this.tickSub.unsubscribe();
    }
  }

  loadVideoencData(): void {
    this.loading = true;
    this.error = null;
    this.http.get<VideoencData>(`${this.apiUrl}/api/videoenc`).subscribe({
      next: (data) => {
        this.videoencData = data;
        this.loading = false;
      },
      error: (err) => {
        this.error = 'Errore nel caricamento dei dati.';
        console.error(err);
        this.loading = false;
      }
    });
  }

  togglePower(): void {
    if (this.isUpdating) return;
    this.isUpdating = true;
    if (this.videoencData) {
      //this.http.post<VideoencData>(`${this.apiUrl}/videoenc`, { state: !this.videoencData.isPowered }).subscribe({
      this.http.post<VideoencData>(`${this.apiUrl}/api/videoenc/power`, { isPowered: this.videoencData.isPowered }).subscribe({
        next: (data) => {
          this.videoencData = data;
          this.isUpdating = false;
        },
        error: (err) => {
          console.error('Errore nell\'aggiornamento dello stato di alimentazione.', err);
          this.isUpdating = false;
        }
      });
    }
  }

    getStateColor(): string {
      if (this.videoencData) {
      switch(this.videoencData.state) {
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
  //   if (this.videoencData) {
  //     switch (this.videoencData.state) {
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

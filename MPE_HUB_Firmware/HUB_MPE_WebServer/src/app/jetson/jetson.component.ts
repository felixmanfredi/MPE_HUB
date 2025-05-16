import { Component, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { JetsonData } from './jetson.model';
import { CommonModule } from '@angular/common';
import { HttpClient, HttpClientModule } from '@angular/common/http';
import { environment } from '../environments/environment';
import { TickService } from '../services/tick.service'; // <-- IMPORTA IL SERVIZIO
import { Subscription } from 'rxjs';

@Component({
  selector: 'app-jetson',
  standalone: true,
  imports: [FormsModule, CommonModule, HttpClientModule],
  templateUrl: './jetson.component.html',
  styleUrls: ['./jetson.component.css']
})
export class JetsonComponent implements OnInit {
  jetsonData: JetsonData | null = null;
  loading = true;
  error: string | null = null;
  isUpdating = false;
  private apiUrl = environment.apiUrl;
  private componentId = environment.jetsonID; // ID UNIVOCO per questo componente
  private tickSub!: Subscription;

  constructor(private http: HttpClient, private tickService: TickService) { }

  ngOnInit(): void {
    this.tickSub = this.tickService.tick$.subscribe(tick => {
      // Esegui la richiesta solo se il tick è allineato con il tuo ID
      if (tick % 4 === this.componentId) {
        this.loadJetsonData();
      }
    });
  }

  ngOnDestroy(): void {
    if (this.tickSub) {
      this.tickSub.unsubscribe();
    }
  }

  loadJetsonData(): void {
    this.loading = true;
    this.error = null;
    this.http.get<JetsonData>(`${this.apiUrl}/api/jetson`).subscribe({
      next: (data) => {
        this.jetsonData = data;
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
    if (this.jetsonData) {
      this.http.post<JetsonData>(`${this.apiUrl}/api/jetson/power`, { isPowered: this.jetsonData.isPowered }).subscribe({
        next: (data) => {
          this.jetsonData = data;
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
    if (this.jetsonData) {
      switch (this.jetsonData.state) {
        case 'normal': return 'green';
        case 'idle': return 'blue';
        case 'watning': return 'orange';
        case 'error': return 'red';
        default: return 'gray';
      }
    }
    return '';
  }

  getStatusColor(isConnected: boolean): string {
    return isConnected ? '#4CAF50' : '#F44336';
  }
}

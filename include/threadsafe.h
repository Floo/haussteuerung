/*Dann kann das Teil Exceptions werfen wie es will, oder auch ein Rcksprung aus einer Funktion mit mehreren returns bringt keine Probleme. 
Sobald das Objekt gel�cht wird, ruft es automatisch Unlock auf, das wiederum Zugriff auf das mutex-Handle hat.

Aber man kann auch als Aufrufer explizit Lock/Unlock von Container aufrufen... das kann je nach Situation Sinn machen, da�die Funktionen intern locken,
da�aber auch der Aufrufer das machen kann. Wer das nicht will, mu�Lock/Unlock in Threadsafe private machen und SingleLock als friend definieren.

Quelle:  http://www.c-plusplus.de/forum/viewtopic-var-t-is-39502-and-start-is-0.html  

Beispiel:
class Container : public Threadsafe{
	private:
		int m_Value;
	public:
          //wenn kein Constructor vorhanden ist, wir wohl automtisch der Defaultconstructor der Basisklasse eingefuegt
          Container()::THREADSAFE(){..}; //falls Constructor der abgeleitetetn Klasse vorhanden ist, und der Basisklassen-Constructor nicht automatisch aufgerufen wird
		void setVal(int value){
			SingleLock slock(this);   // entweder explizit locken
			slock->Lock();
			m_Value = value;
			slock->Unlock();
		};
		int getVal(){                     // oder implizit
			SingleLock slock(this, true); // ab hier ist gesperrt
			return m_Value;
			// hier wird automatisch aufger�mt
	};
};*/

#ifndef THREADSAFE_H
#define THREADSAFE_H
    
#include <pthread.h>

class Threadsafe{
	private:
		pthread_mutex_t m_code_access;
	protected:
		Threadsafe(void){pthread_mutex_init(&m_code_access, NULL);};
		~Threadsafe() {};
	public:
		void Lock(){pthread_mutex_lock(&m_code_access);};
		void Unlock(){pthread_mutex_unlock(&m_code_access);};
};

class SingleLock{
	private:
		Threadsafe *m_pThreadsafe;
	public:
		SingleLock(Threadsafe *threadsafe, bool lock = false){
			m_pThreadsafe = threadsafe;
			if(lock) Lock();
		}
		~SingleLock(){Unlock();};
	void Lock(){
		if(m_pThreadsafe) m_pThreadsafe->Lock();
	}
	void Unlock(){  
		if(m_pThreadsafe) m_pThreadsafe->Unlock();
	}
};

#endif
